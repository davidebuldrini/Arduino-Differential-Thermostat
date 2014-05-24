#include <thermostat.h>


TempSensor::TempSensor(char *name, byte pin) 
{
  this->pin = pin;
  this->name = name;
  linkedOutputPin = NULL;
  calibrate(20); // calibrate the sensor by default to 20°
}

// return the current temperature detected by the sensor
float TempSensor::value() {
			
	// calculate temperature: T = Tref + (Vref − Vout) / 2
	// -2 mV = -1°
	// max V with internal reference: 1,1 V
	// 1,1 / 1024 = 0,001 = 1mV
	// -2 Apin = -1°
			
	#if defined(TEMPERATURE_BYTE)
		return (byte) Tref + ((Vref - raw()) / 2);		
	#else
		return (float) Tref + ((Vref - raw()) / 2);		
	#endif
}

// Adjust the coefficient relying upon given temperature  
#if defined(TEMPERATURE_BYTE)
	void TempSensor::calibrate(byte temperature) {
#else 
	void TempSensor::calibrate(float temperature) {
#endif
	
	Vref = raw(); // analog pin sampled voltage at calibration time
	Tref = temperature; // temperature at calibration time
}

int TempSensor::raw() {
	return analogRead(pin);
}

void TempSensor::linkOutput(byte pin, byte defaultState) {
	
	linkedOutputPin = pin;
	pinMode(pin, OUTPUT);    
    digitalWrite(pin, HIGH);     
}

Thermostat::Thermostat() {
  sensorsCount = 0;
  
  // to avoid relay switch flickering 
  switchDelay = 30000; 
  _state = 1; 
  
  for (byte c = 0; c < MAX_SENSORS_COUNT; c++) {	  
	  _timer[c] = millis();
  }	
	
}

// add a sensor to the sensors array and return the sensor id
byte Thermostat::addSensor(TempSensor *sensor) {
  
  // inizialize a new sensor object and put it in the sensors array
  sensors[sensorsCount] = sensor;
  sensorsCount ++;
  return sensorsCount - 1; // newly added sensor id
}   

// add a reference sensor to the sensors array and return the sensor id
byte Thermostat::addReference(TempSensor *sensor) {
  
  reference = sensor;
  return addSensor(sensor);
}


boolean Thermostat::checkDelta() {
	
	/*
	 * over delta t ? onOvercome callback, wait for switch delay timer
	 * under delta t? onNormal callback
	*/
	
	if (_state == 0) {
		  return false;
	}	
	  
	_allNormal = true;
	boolean toNormal = false; 
	  
	// Cycle all sensors
	for (byte c = 0; c < sensorsCount; c++) {
	
		if (!isReference(c)) { // exclude the reference sensor
			
			if ((unsigned long)(millis() - _timer[c]) >= switchDelay) {
										
				if (!overcome[c]) {		
					// sensor is overcoming delta t  
					if (deltaT(c) > triggerDeltaT) {

						overcome[c] = true;
						onOvercome(sensors[c], this); // overcome callback		
						_timer[c] = millis();				
					}
				}
				// sensor already overcomed delta t		
				else {
					// a previously overcomed sensor now returns to normal state 
					if (deltaT(c) <= triggerDeltaT ) {
				
						overcome[c] = false;						
						onNormal(sensors[c], this); // fall to normal callback
						toNormal = true;
					}
			    }
			}														
			
			if (overcome[c]) {
				_allNormal = false;
			}
		}
	}
	
   
	if (toNormal && _allNormal) {		
		onAllNormal(this);
	}
	return true;		
}

// return the delta t between the sensor parameter and the reference sensor
float Thermostat::deltaT(byte sid) {
	
	return sensors[sid]->value() - reference->value();
}

// tell wheter a sensor is a reference sensor or not
boolean Thermostat::isReference(byte sid) {
	
	if (reference == sensors[sid]) {
		return true;
	}
	else {
		return false;
	}
}

void Thermostat::setState(byte state) {
	_state = state;
}


