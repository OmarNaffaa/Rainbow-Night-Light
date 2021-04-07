// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include <stdlib.h>

// global constants
constexpr uint8_t third_sseg_dp = 0x04;
constexpr double firstPwmStep = 167.0;

// global core instantiations
GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));

// function prototypes
void adcVoltageToSseg(SsegCore *sseg_p, double adcVal);
void rgbLedColor(PwmCore *pwm_p, double adcVal);

enum RgbChannel {blue = 0, green, red};

int main()
{
	double adcReading = 0;

	while(1)
	{
		// obtain voltage reading from XADC module
		adcReading = adc.read_adc_in(0) * 100;

		// SSeg display logic
		adcVoltageToSseg(&sseg, adcReading);

		// RGB led color logic
		rgbLedColor(&pwm, adcReading);

		sleep_ms(100);
	}
}

void adcVoltageToSseg(SsegCore *sseg_p, double adcVal)
{
	/*
	   Displays the obtained voltage on the SSEG in the format "x.x"
	*/

	// turn off unneeded SSeg displays
	for (int i = 7; i > 2; --i)
		sseg_p->write_1ptn(0xff, i);

	// set constant Sseg displays
	sseg_p->write_1ptn(sseg_p->h2s(0), 2);
	sseg_p->set_dp(third_sseg_dp);

	// determine value for whole portion of adc value
	int upperDispVal = (int) (adcVal / 10.0);
	uint8_t convertedUpperDispVal = sseg_p->h2s(upperDispVal);
	sseg_p->write_1ptn(convertedUpperDispVal, 1);

	// determine value for fractional portion of adc value
	int lowerDispVal = ((int) adcVal) % 10;
	uint8_t convertedLowerDispVal = sseg_p->h2s(lowerDispVal);
	sseg_p->write_1ptn(convertedLowerDispVal, 0);
}

void rgbLedColor(PwmCore *pwm_p, double adcVal)
{
	/*
	   Determines brightness for red, green, and blue leds to
	   produce varying colors on the RGB led

	   (maps voltage reading from 0 --> 999 instead of 0 --> 1)
	*/

	int adcVolts = (int) adcVal * 10;

	// determine red led brightness
	double redDuty = 1.0, greenDuty = 1.0, blueDuty = 1.0;

	if (adcVolts <= firstPwmStep) // x <= 167
	{
		redDuty = 1.0;
		greenDuty = 1.0 - (firstPwmStep - ((double)adcVolts))/firstPwmStep;
		blueDuty = 0.0;
	}
	else if (adcVolts > firstPwmStep && adcVolts <= firstPwmStep*2) // 168 < x <= 334
	{
		redDuty = (firstPwmStep*2 - ((double)adcVolts))/(firstPwmStep*1);
		greenDuty = 1.0;
		blueDuty = 0.0;
	}
	else if (adcVolts > firstPwmStep*2 && adcVolts <= firstPwmStep*3) // 335 < x <= 501
	{
		redDuty = 0.0;
		greenDuty = 1.0;
		blueDuty = 1.0 - (firstPwmStep*3 - ((double)adcVolts))/firstPwmStep;
	}
	else if (adcVolts > firstPwmStep*3 && adcVolts <= firstPwmStep*4) // 502 < x <= 668
	{
		redDuty = 0.0;
		greenDuty = (firstPwmStep*4 - ((double)adcVolts))/(firstPwmStep);
		blueDuty = 1.0;
	}
	else if (adcVolts > firstPwmStep*4 && adcVolts <= firstPwmStep*5) // 669 < x <= 835
	{
		redDuty = 1.0 - (firstPwmStep*5 - ((double)adcVolts))/(firstPwmStep);
		greenDuty = 0.0;
		blueDuty = 1.0;
	}
	else // 836 < x
	{
		redDuty = 1.0;
		greenDuty = 0.0;
		blueDuty = (firstPwmStep*6 - ((double)adcVolts))/(firstPwmStep);
	}

	// set duty cycles
	pwm_p->set_duty(redDuty, red);
	pwm_p->set_duty(greenDuty, green);
	pwm_p->set_duty(blueDuty, blue);

}
