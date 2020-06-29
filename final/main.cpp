#include "mbed.h"
#include "bbcar.h"

#define duration 1.5

Serial pc(USBTX, USBRX);
Serial uart(D1, D0);   // tx, rx
Serial xbee(D12, D11); // Xbee

PwmOut pin9(D9), pin8(D8);
DigitalInOut pin13(D13);
DigitalIn pin3(D3);
DigitalOut led3(LED3);
DigitalOut led2(LED2);
DigitalOut led1(LED1);

Ticker servo_ticker;
Ticker encoder_ticker;
Timer t;

BBCar car(pin8, pin9, servo_ticker);
parallax_encoder encoder0(pin3, encoder_ticker);

float data[50]; // the array to save the ping data in Mission 2
int flag = 0;
int flags = 0;

void straight(int speed, int len)
{
    encoder0.reset();
    car.goStraight(-speed);
    while (encoder0.get_cm() < len)
        wait_ms(50);
    car.stop();
    wait(0.5);
}

void left(char dir)
{
    int speed = 70;
    float dur = 0.07;
    if (dir == 'b')
    {
        speed = -speed;
        dur += 0.01;
    }

    car.turn(speed, 0.1); // turn left
    wait(duration + dur);
    car.stop();
    wait(0.5);
}

void right(char dir)
{
    int speed = 70;
    float dur = 0.07;
    if (dir == 'b')
    {
        speed = -speed;
    }

    car.turn(speed, -0.1); // turn right
    wait(duration + dur);
    car.stop();
    wait(0.5);
}

int main()
{
    parallax_ping ping1(pin13);

    double pwm_table0[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
    double speed_table0[] = {-17.305, -16.82, -15.631, -12.202, -6.141, 0.000, 6.141, 12.202, 15.631, 16.827, 17.305};
    double pwm_table1[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
    double speed_table1[] = {-16.030, -15.551, -14.275, -11.005, -5.024, 0.000, 5.024, 11.005, 14.275, 15.551, 16.030};

    car.setCalibTable(11, pwm_table0, speed_table0, 11, pwm_table1, speed_table1);

    // mission1

    // set led
    led1 = 1;
    led2 = 1;
    led3 = 1;

    // go straight until find data matrix
    xbee.printf("go straight until find data matrix\n\r");
    straight(70, 120);

    // turn left after reading data matrix
    xbee.printf("turn left after reading data matrix\n\r");
    right('b');

    // go straight to designated position, and get ready to reverse parking (the 0 spot)
    xbee.printf("go straight to designated position, and get ready to reverse parking (the 0 spot)\n\r");
    straight(70, 68);

    // reverse parking
    xbee.printf("reverse parking\n\r");
    left('b');
    straight(-70, 30);

    // leave parking spot (just go straight)
    xbee.printf("leave parking spot (just go straight)\n\r");
    straight(70, 25);

    // turn right and go straight to taking picture spot
    xbee.printf("turn right and go straight to taking picture spot\n\r");
    right('f');
    straight(80, 20);

    // trun to take the picture
    xbee.printf("trun to take the picture\n\r");
    right('b');
    straight(-70, 10);

    // take picture
    xbee.printf("take picture\n\r");
    wait(0.5);
    char s[21];
    sprintf(s, "image_classification\n\r");
    uart.puts(s);
    wait(3);

    // leave take picture spot
    xbee.printf("leave take picture spot\n\r");
    straight(70, 13);
    right('f');
    straight(80, 43);

    // turn right, go stranght, and turn right
    xbee.printf("turn right, go stranght, and turn right\n\r");
    left('b');
    straight(70, 115);
    left('b');

    // mission 2

    // set led
    led2 = 0;

    // go straight, turn right, and go straight a little (the 2 item)
    xbee.printf("go straight, turn right, and go straight a little (the 2 item)\n\r");
    straight(70, 50);
    left('b');
    straight(70, 30);

    // pin scan
    xbee.printf("pin scan\n\r");
    for (int i = 0; i < 50; i++)
    {
        car.turn(-30, 0.1); // turn left
        data[i] = ping1;
        wait(0.02);
    }
    car.turn(32, 0.1); // turn left
    wait(1);
    car.stop();
    wait(0.5);

    // go back and turn back
    xbee.printf("go back and turn back\n\r");
    straight(-70, 25);
    left('f');

    // go straight and turn right
    xbee.printf("go straight and turn right\n\r");
    straight(70, 25);
    left('b');

    // mission3

    //set led
    led3 = 1;
    led1 = 0;

    // finally, go straight to the end
    xbee.printf("finally, go straight to the end\n\r");
    straight(70, 140);

    // send to xbee
    flag = 0;
    int change = 0;
    float acc[50] = {0};

    for (int i = 0; i < 49; i++)
    {
        acc[i] = data[i + 1] - data[i];
    }

    for (int i = 0; i < 50; i++)
    {
        xbee.printf("%1.4f\n\r", data[i]);

        if (i != 49 && acc[i] * acc[i + 1] < 0)
            change = 1;

        wait(0.1);
    }

    if (acc[40] > 0) 
        flag = 0;

    if (acc[40] < 0.04) // rect
        flag = 1;

    if (acc[40] < 0)  
        flag = 2;

    if (!flags)
        xbee.printf("right triangle\n\r");
    else if (flags == 1)
        xbee.printf("rectangle\n\r");
    else if (flags == 3 && change)
        xbee.printf("triangle\n\r");
    else
        xbee.printf("M\n\r");
}
