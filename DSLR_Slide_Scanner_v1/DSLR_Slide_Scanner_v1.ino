/**
 * DSLR Slide scanner
 *
 * Description:
 * Auto scan photographic slides using a
 * DSLR camera and one ARDUINO UNO REV.3
 *
 * Code forked from Koen Dooms. Thanx!
 * Modified by Dan Nilsson 2014-02-25
 *
 */

// Amount of slides to scan
int slidesToScan					= 5;		// There are aprox 50 slide in each tray


// Button control
const int runLedPin					= 13;		// Scanning = ON
volatile const int startStopPin		= 3;		// Start/stop button
volatile const int bounceDelayTime	= 5;		// Time in usec to wait for debounce action. ???
volatile boolean runningState		= false;	// False = no scan, True = scan running


// Projector control
const int projectorLampRelay		= 11;		// Arduino pin for projector lamp relay
const int projectorRelay			= 8;		// Arduino pin for projector slider relay
const int nextSlideTime				= 250;		// Time in msec that relay should be active to go to next slide
const int slideMoveTime				= 2000;		// Time in msec to wait for slide to move into place
const int slideDelayTime			= 1500;		// Time in msec to wait between slide activations


// DSLR control
const int focusRelay				= 9;		// Arduino pin for camera focus
const int shutterRelay				= 10;		// Arduino pin for camera shutter
const int focusTime					= 100;		// Time in msec focus is activated before shutter is activated
const int shutterTime				= 200;		// Time in msec shutter is activated


// Status LED vars
long cycleCount						= 0;		// Keep track of cycles in loop
int cycleDivider					= 10000;	// After how many cycles the status LED will change
boolean	ledStatus					= false;	// LED will be off to start with


// Setup magoo
void setup()
{
	// Initialize pin so relay is inactive at reset
	digitalWrite(projectorLampRelay, LOW);
	digitalWrite(projectorRelay, LOW);
	digitalWrite(focusRelay, LOW);
	digitalWrite(shutterRelay, LOW);

	pinMode(startStopPin, INPUT);				// Set pin for start/stop button
	pinMode(runLedPin, OUTPUT);					// Set pin for indicator LED

	pinMode(projectorLampRelay, OUTPUT);		// Set projector relay pin as output
	pinMode(projectorRelay, OUTPUT);			// Set projector relay pin as output
	pinMode(focusRelay, OUTPUT);				// Set focus relay pin as output
	pinMode(shutterRelay, OUTPUT);				// Set shutter relay pin as output

	// Make sure relay is inactive at reset
	delay(1000);

	digitalWrite(runLedPin, ledStatus);			// Turn LED off
	attachInterrupt(1, startPause, RISING);		// Enable interrupt for start/pause button
}

// The loop routine runs over and over again forever
void loop()
{
	// Wait for button to be pressed
	while(runningState == false) {
		digitalWrite(runLedPin, runningState);
	}

	digitalWrite(runLedPin, runningState);

	// LED lit during scan
	digitalWrite(runLedPin, HIGH);

	// Lit projector lamp
	digitalWrite(projectorLampRelay, HIGH);

	// Lamp warming up
	//delay(2000);

	// Start scanning
	for( int i = 0; ( i < slidesToScan ) && ( runningState == true ); i++ )
	{
		slideMove(nextSlideTime, slideMoveTime);

		shutterRelease(focusTime, shutterTime);

		delay(slideDelayTime);
	}

	// Done scanning, turn off the lamp
	digitalWrite(projectorLampRelay, LOW);

	// Move forward some slide to insert new cassete
	for( int k = 0; k < 5; k++ )
	{
		slideMove(nextSlideTime, 500);
	}

	// Loop done
	runningState = false;

	/*if(runningState)								// Start scanning
	{
		digitalWrite(runLedPin, runningState);		// LED lit during scan

		digitalWrite(projectorLampRelay, HIGH);

		cycleCount = 0;								// Reset cycle count that controls LED blinking

		slideMove(nextSlideTime, slideMoveTime);	// Move to next or previous slide
		shutterRelease(focusTime, shutterTime);		// Focus and shutter are activated

		slidesToScan--;

		if(slidesToScan <= 0)
		{
			runningState = false;

			// Wait a bit for camera to take the last shot
			delay(1000);

			// Turn of to save the lamp
			digitalWrite(projectorLampRelay, LOW);

			slidesToScan = slidesToScanRestart;	
		}

		if(runningState)
		{
			delay(slideDelayTime);					// If still running, wait for x msec before proceeding to next slide
		}
	}
	else
	{
		slidesToScan = slidesToScanRestart;
		digitalWrite(projectorLampRelay, LOW);
		blinkLED(runLedPin);						// Blink status LED to indicate we are ready
	}*/
}


// Start/pause button press interrupt ISR - change the running state
void startPause()
{
	// Button was pressed, now wait a bit to check state again (soft debounce)
	for( int j = 0; j < bounceDelayTime; j++ )
	{
		delayMicroseconds(1000);
	}

	if( digitalRead(startStopPin) == HIGH )
	{
		// If still pressed, then change running status
		runningState = !runningState;
	}
}


// Projector slide mover
void slideMove( int projectorDelay, int waitDelay )
{
	digitalWrite(projectorRelay, HIGH);
	delay(projectorDelay);

	digitalWrite(projectorRelay, LOW);
	delay(waitDelay);
}


// Camera shutter release
void shutterRelease( int focusDelay, int shutterDelay )
{
	// skip focus if value set to zero
	if(focusDelay != 0)
	{
		digitalWrite(focusRelay, HIGH);
		delay(focusDelay);
	}

	// skip shutter if value set to zero
	if(shutterDelay != 0)
	{
		digitalWrite(shutterRelay, HIGH);
		delay(shutterDelay);
	}

	digitalWrite(shutterRelay, LOW);	// First release shutter
	digitalWrite(focusRelay, LOW);		// Last release focus
}


// LED blinking, waiting for user to press start button
void blinkLED(int ledPin)
{
	cycleCount++;

	if( cycleCount % cycleDivider == 0)
	{
		ledStatus = !ledStatus;
		digitalWrite(runLedPin, ledStatus);
	}
}







