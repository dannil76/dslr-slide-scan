/**
 * DSLR Slide scanner
 *
 * Description:
 * Auto scan photographic slides using a DSLR camera
 * and a slide projector controlled by ARDUINO UNO REV.3
 *
 */

#include <PciManager.h>
#include <SoftTimer.h>
#include <Debouncer.h>
#include <BlinkTask.h>
#include <DelayRun.h>


// There are aprox 50 slides in each tray
volatile const int slidesToScan				= 50;
volatile int slideCount;

// Button
volatile const int BUTTON_PIN			 	= 3;
volatile int buttonClicked					= LOW;


// Button press listener
Debouncer debBtn(
	BUTTON_PIN,
	MODE_OPEN_ON_PUSH,
	onPress,
	onRelease
);
volatile unsigned long btnHoldtimeProjLamp	= 2000;		// If button is pressed for x msec, lit projector lamp only.
volatile unsigned long time;
volatile boolean isLampConfiged				= false;

// Status LED
volatile const int STATE_LED_PIN 			= 13;
volatile const int STATE2_LED_PIN 			= 7;

BlinkTask scanLED( STATE2_LED_PIN, 250 );
BlinkTask standbyLED( STATE_LED_PIN, 500, 1000 );
BlinkTask stopLED( STATE_LED_PIN, 100, 100, 4 );


// Projector
volatile const int PROJ_LAMP_PIN			= 11;		// Arduino pin (relay 1)
volatile const int PROJ_BUTTON_PIN		 	= 8;		// Arduino pin (relay 4)

volatile const int projPressTime			= 200;		// Longer will reverse the movement
volatile const int slideInPlaceTime			= 2500;		// Wait for slide to move into place (msec)
volatile const int slideDelayTime			= 100;		// Time between slides (msec)


// DSLR Camera
volatile const int FOCUS_PIN				= 9;		// Arduino pin for camera focus (relay 3)
volatile const int SHUTTER_PIN				= 10;		// Arduino pin for camera shutter (relay 2)

volatile const int focusPressTime			= 200;		// Time in msec focus is activated before shutter is activated
volatile const int shutterPressTime			= 400;		// Time in msec shutter is activated


/**
 * Delayed task decalaration
 *
 * Flow bottom up :)
 *
 */

// Ending sequence
DelayRun endScanTask( 5000, CB_endScanTask );
DelayRun moveSlides( 1500, CB_moveSlides, &endScanTask );

BlinkTask slideMoverEnd( PROJ_BUTTON_PIN, projPressTime, 1250, 1 );

// DSLR Camera flow
DelayRun eTask( shutterPressTime, 	CB_eTask);				// 4 Release focus and shutter
DelayRun dTask( focusPressTime, 	CB_dTask, &eTask);		// 3 Press shutter
DelayRun cTask( slideInPlaceTime, 	CB_cTask, &dTask);		// 2 Press and hold focus

// Projector flow
DelayRun bTask( projPressTime, 		CB_bTask, &cTask);		// 1b Let go of the projector button to feed next slide
DelayRun aTask( slideDelayTime, 	CB_aTask, &bTask);		// 1a Press projector button

DelayRun startScan( 2000, NULL, &aTask );					// Wait a bit for the lamp to warm up then run the magic


// Listen for button press
Task buttonPressTime( 100, CB_buttonPressTime );

// Insert one slide so we can adjust camera
BlinkTask slideMoverLampConfig( PROJ_BUTTON_PIN, projPressTime, 0, 1 );
BlinkTask slideMoverBackLampConfig( PROJ_BUTTON_PIN, 600, 0, 1 );

/**
 * Arduino init setup
 *
 */

void setup()
{
	// Button
	digitalWrite(BUTTON_PIN, LOW);
	pinMode(BUTTON_PIN, INPUT);

	// Status LED
	digitalWrite(STATE_LED_PIN, LOW);
	digitalWrite(STATE2_LED_PIN, LOW);
	pinMode(STATE_LED_PIN, OUTPUT);
	pinMode(STATE2_LED_PIN, OUTPUT);

	// Projector
	digitalWrite(PROJ_LAMP_PIN, LOW);
	digitalWrite(PROJ_BUTTON_PIN, LOW);
	pinMode(PROJ_LAMP_PIN, OUTPUT);
	pinMode(PROJ_BUTTON_PIN, OUTPUT);

	// DSLR Camera
	digitalWrite(FOCUS_PIN, LOW);
	digitalWrite(SHUTTER_PIN, LOW);
	pinMode(FOCUS_PIN, OUTPUT);
	pinMode(SHUTTER_PIN, OUTPUT);

	// Register button
	PciManager.registerListener( BUTTON_PIN, &debBtn );

	//Serial.begin(9600);

	delay(1000);

	//Serial.println("READY!");

	// Ready
	standbyLED.start();
}


/**
 * DelayRun callback functions
 *
 */

boolean CB_endScanTask(Task* task)				// 6 All ready for the next batch
{
	// Remove timers
	SoftTimer.remove(&startScan);
	SoftTimer.remove(&aTask);
	SoftTimer.remove(&bTask);
	SoftTimer.remove(&cTask);
	SoftTimer.remove(&dTask);
	SoftTimer.remove(&eTask);
	/*SoftTimer.remove(&moveSlides);
	SoftTimer.remove(&endScanTask);*/

	// Turn off relays
	digitalWrite(PROJ_BUTTON_PIN, LOW);
	digitalWrite(FOCUS_PIN, LOW);
	digitalWrite(SHUTTER_PIN, LOW);
	digitalWrite(STATE2_LED_PIN, LOW);

	// Ready for the next batch of slides
	scanLED.stop();
	stopLED.start();
	standbyLED.start();

	return false;
}

boolean CB_moveSlides(Task* task)				// 5 Ending sequence
{
	digitalWrite(PROJ_LAMP_PIN, LOW);
	slideMoverEnd.start();
	return true;
}

boolean CB_eTask(Task* task)					// 4 Release focus and shutter take the shot!
{
	//Serial.println("Release focus and shutter button. Take the shot!");

	digitalWrite(SHUTTER_PIN, LOW);
	digitalWrite(FOCUS_PIN, LOW);

	slideCount++;

	// Stop when slide to scan amount is met
	if( slideCount >= ( slidesToScan + 1 ) )
	{
		/*Serial.print("All ");
		Serial.print(slidesToScan);
		Serial.println(" slides scanned!");
		Serial.println();*/

		// Restart scan on button press
		buttonClicked = !buttonClicked;

		// Start ending sequence
		moveSlides.startDelayed();

		return false;
	}
	else if( slideCount >= slidesToScan )
	{
		digitalWrite(PROJ_LAMP_PIN, LOW);
	}

	return true;
}

boolean CB_dTask(Task* task)					// 3 Press shutter
{
	//Serial.println("Pressing shutter button");

	digitalWrite(SHUTTER_PIN, HIGH);
	return true;
}

boolean CB_cTask(Task* task)					// 2 Press and hold focus
{
	//Serial.println("Pressing focus button");

	digitalWrite(FOCUS_PIN, HIGH);
	return true;
}

boolean CB_bTask(Task* task)					// 1b Let go of the projector button to feed next slide
{
	digitalWrite(PROJ_BUTTON_PIN, LOW);
	return true;
}

boolean CB_aTask(Task* task)					// 1a Press projector button
{
	//Serial.println("Move to next slide");

	digitalWrite(STATE2_LED_PIN, LOW);
	digitalWrite(PROJ_BUTTON_PIN, HIGH);
	return true;
}


// Listen for long button press
void CB_buttonPressTime(Task* task)
{
	if( ( millis() - time ) > btnHoldtimeProjLamp )
	{
		/*Serial.print("Button pressed for ");
		Serial.print(btnHoldtimeProjLamp / 1000);
		Serial.println(" sec. Turning on projector lamp only.");*/

		isLampConfiged = true;

		standbyLED.stop();
		scanLED.stop();
		digitalWrite(STATE_LED_PIN, LOW);
		digitalWrite(STATE2_LED_PIN, HIGH);
		digitalWrite(PROJ_LAMP_PIN, HIGH);
	}
}


// Listen for button event
void onPress()
{
	if( slideCount < 1 )
	{
		time = millis();
		SoftTimer.add(&buttonPressTime);
	}

	buttonClicked = !buttonClicked;
}

void onRelease(unsigned long timeMs)
{
	SoftTimer.remove(&buttonPressTime);
	slideCount = 0;

	if( timeMs < btnHoldtimeProjLamp )			// Long button press = lit projector lamp only. Good for camera setup.
	{

		if(buttonClicked)
		{
			// Enlight the sky
			scanLED.start();
			standbyLED.stop();

			digitalWrite(STATE_LED_PIN, LOW);
			digitalWrite(PROJ_LAMP_PIN, HIGH);

			// Close the loop
			eTask.followedBy = &aTask;

			// Go go go!
			startScan.startDelayed();
		}
		else
		{
			if(isLampConfiged)
			{
				slideMoverBackLampConfig.start();
				isLampConfiged = false;
			}

			digitalWrite(PROJ_LAMP_PIN, LOW);
			CB_endScanTask(NULL);
		}

	}
	else
	{
		slideMoverLampConfig.start();
	}
}