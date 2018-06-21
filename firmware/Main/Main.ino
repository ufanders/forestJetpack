#include <Event.h>
#include <Timer.h>

#include <Debounce.h>

#include <FastLED.h>

#define CHIPSET     DOTSTAR
#define DATA_PIN     10
#define CLOCK_PIN     16
#define COLOR_ORDER BGR
#define SPI_SPEED     5
#define NUM_LEDS    60

#define BRIGHTNESS  50
#define FRAMES_PER_SECOND 60

bool gReverseDirection = false;

CRGB leds[NUM_LEDS*2]; //we hack in another strip worth of data.

// Fire2012 with programmable Color Palette
//
// This code is the same fire simulation as the original "Fire2012",
// but each heat cell's temperature is translated to color through a FastLED
// programmable color palette, instead of through the "HeatColor(...)" function.
//
// Four different static color palettes are provided here, plus one dynamic one.
// 
// The three static ones are: 
//   1. the FastLED built-in HeatColors_p -- this is the default, and it looks
//      pretty much exactly like the original Fire2012.
//
//  To use any of the other palettes below, just "uncomment" the corresponding code.
//
//   2. a gradient from black to red to yellow to white, which is
//      visually similar to the HeatColors_p, and helps to illustrate
//      what the 'heat colors' palette is actually doing,
//   3. a similar gradient, but in blue colors rather than red ones,
//      i.e. from black to blue to aqua to white, which results in
//      an "icy blue" fire effect,
//   4. a simplified three-step gradient, from black to red to white, just to show
//      that these gradients need not have four components; two or
//      three are possible, too, even if they don't look quite as nice for fire.
//
// The dynamic palette shows how you can change the basic 'hue' of the
// color palette every time through the loop, producing "rainbow fire".

byte coolingValue;
byte sparkingValue;
byte brightnessValue;

CRGBPalette16 gPal0;
CRGBPalette16 gPal1;
CRGBPalette16 gPal2;
CRGBPalette16 gPal3;
CRGBPalette16* gPalCurrent;

void doSomething(void* context);

//======== debounce stuff
byte button1 = A0;
byte button2 = A1;
bool button1Down = false;
bool button2Down = false;
Debounce Button1(button1); // Button1 debounced, default 50ms delay.
Debounce Button2(button2); // Button2 debounced, default 50ms delay.

//======== timer stuff.
//Timer tBlower;
//Timer tVape;
int smokeEvent;
int vapeEvent;

//======== control stuff
byte en_blower = 15;
byte en_vape = 14;
byte colorMode = 0;
byte smokeFlag = 0;
uint32_t timerTicksBase, timerTicks1, timerTicks2;

void setup() {
  delay(1000); // sanity delay

  timerTicksBase = 0;
  timerTicks1 = 0;
  timerTicks2 = 0;
  
  //we hack in twice as many slots for LEDs given we have 2 strips connected in series.
  FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, (NUM_LEDS*2)).setCorrection( TypicalLEDStrip );
  brightnessValue = BRIGHTNESS;
  FastLED.setBrightness(brightnessValue);

  // This first palette is the basic 'black body radiation' colors,
  // which run from black to red to bright yellow to white.
  //gPal = HeatColors_p;
  
  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  gPal1 = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  gPal2 = CRGBPalette16( CRGB::Black, CRGB::Purple, CRGB::Aqua,  CRGB::White);
  
  // Third, here's a simpler, three-step gradient, from black to red to white
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);

  gPal0 = CRGBPalette16( CRGB::Black, CRGB::White);

  coolingValue = 55;
  sparkingValue = 120;
  colorMode = 1;

  CRGB colorInit;
  colorInit = ColorFromPalette( gPal0, 0);
  for(int i = 0; i < (NUM_LEDS*2); i++)
  {
    leds[i] = colorInit;
  }
  FastLED.show();

  //pinMode(9, OUTPUT); //heartbeat indicator.
  pinMode(button1, INPUT_PULLUP); // Watch for the PULLUP
  pinMode(button2, INPUT_PULLUP); // Watch for the PULLUP
  digitalWrite(en_blower, 0);
  digitalWrite(en_vape, 0);
  pinMode(en_blower, OUTPUT);
  pinMode(en_vape, OUTPUT);

  //int tickEvent = tBlower.every((1000/60), doSomething, (void*)2);

  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 1092; //60Hz //31250;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}


ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
//void doSomething(void* context)
{
  //top half of ISR.
  
  timerTicksBase++;

  if(1) //timerTicksBase % 100)
  {
    timerTicks1++;
  }

  if(timerTicksBase == 100) timerTicksBase = 0; //reset timebase.
  
  //digitalWrite(9, digitalRead(9) ^ 1); //toggle heartbeat indicator.
}

 
void loop()
{

  //tBlower.update(); //update timer.
  //tVape.update(); //update timer.
  
  //TODO: read pin states.
  if(!Button1.read() && button1Down == false)
  {
    //LED mode button pushed.
    button1Down = true;
    
    colorMode++;
    if(colorMode > 3) colorMode = 0;

    switch(colorMode)
    {
      case 0: //LEDs off.
      coolingValue = 55;
      sparkingValue = 192;
      brightnessValue = 0;
      FastLED.setBrightness(brightnessValue);
      break;

      case 1: //Fire.
      coolingValue = 55;
      sparkingValue = 100;
      brightnessValue = 96;
      FastLED.setBrightness(brightnessValue);
      break;

      case 2: //MDMA fire.
      coolingValue = 55;
      sparkingValue = 160;
      brightnessValue = 192;
      FastLED.setBrightness(brightnessValue);
      break;

      case 3: //FREAK OUT
      coolingValue = 55;
      sparkingValue = 192;
      brightnessValue = 255;
      FastLED.setBrightness(brightnessValue);
      break;
    }
  }

  if(Button1.read())
  {
    button1Down = false;
  }

  if(!Button2.read() && button2Down == false)
  {
    //smoke mode button pushed.
    button2Down = true;

    smokeFlag ^= 1; //toggle smoke flag.

    if(smokeFlag)
    {
      //smokeEvent = tBlower.oscillate(en_blower, 10000, HIGH);
      //vapeEvent = tBlower.oscillate(en_vape, 10000, HIGH);

      digitalWrite(en_blower, 1);
      digitalWrite(en_vape, 1);
    }
    else
    {
      //tBlower.stop(smokeEvent);
      //tBlower.stop(vapeEvent);

      digitalWrite(en_blower, 0);
      digitalWrite(en_vape, 0);
    }
  }

  if(Button2.read())
  {
    button2Down = false;
  }
  
  //bottom half of ISR.

  if((timerTicks1 >= 1) /*&& (colorMode > 0)*/)
  {
    //TODO: draw video frame.
    
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy( random());
  
    // Fourth, the most sophisticated: this one sets up a new palette every
    // time through the loop, based on a hue that changes every time.
    // The palette is a gradient from black, to a dark color based on the hue,
    // to a light color based on the hue, to white.
    //

    if(colorMode == 3)
    {
      static uint8_t hue = 0;
      hue++;
      CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
      CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
      gPal3 = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);
    }
  
    Fire2012WithPalette(); // run simulation frame, using palette colors
    
    FastLED.show(); // display this frame
    //FastLED.delay(1000 / FRAMES_PER_SECOND);
    
    timerTicks1 = 0; //reset this timer.
  }

  if(timerTicks2 == 10)
  {
    //TODO: update state of indicators?

    timerTicks2 = 0; //reset this timer.
  }
  
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120


void Fire2012WithPalette()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((coolingValue * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < sparkingValue ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);

      CRGB color;

      switch(colorMode)
      {
        case 0: //all black.
          color = ColorFromPalette( gPal0, 0);
        break;

        case 1: //fire.
          color = ColorFromPalette( gPal1, colorindex);
        break;

        case 2: //MDMA fire.
          color = ColorFromPalette( gPal2, colorindex);
        break;

        case 3: //FREAK OUT
          color = ColorFromPalette( gPal3, colorindex);
        break;
      }
      
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      
      leds[pixelnumber] = color;
      leds[pixelnumber+NUM_LEDS] = color; //we hack in a copy of the first strip data into the second strip.
    }
}


