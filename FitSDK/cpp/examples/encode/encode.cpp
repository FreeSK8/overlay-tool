////////////////////////////////////////////////////////////////////////////////
// The following FIT Protocol software provided may be used with FIT protocol
// devices only and remains the copyrighted property of Garmin Canada Inc.
// The software is being provided on an "as-is" basis and as an accommodation,
// and therefore all warranties, representations, or guarantees of any kind
// (whether express, implied or statutory) including, without limitation,
// warranties of merchantability, non-infringement, or fitness for a particular
// purpose, are specifically disclaimed.
//
// Copyright 2008-2016 Garmin Canada Inc.
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include "math.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>      // std::get_time
#include <ctime>  
#include <fstream>
#include <cstdlib>

#include "fit_encode.hpp"
#include "fit_mesg_broadcaster.hpp"
#include "fit_file_id_mesg.hpp"
#include "fit_date_time.hpp"

using namespace std;

// Number of semicircles per meter at the equator
#define SC_PER_M 107.173


class DataPoint {
   public:
      double vin;
      double tempMotor;
      double tempMotor2;
      double tempMotor3;
      double tempMotor4;
      double tempESC;
      double tempESC2;
      double tempESC3;
      double tempESC4;
      double dutyCycle;
      double currentMotor;
      double currentMotor2;
      double currentMotor3;
      double currentMotor4;
      double currentBattery;
      double speed;
      int eRPM;
      double distance;
      int eDistance;
      double lat;
      double lon;
      double altitude;
      double speedGPS;
      int satellites;
      int fault;
      double wattHours;
      double wattHoursRegen;
};
typedef std::map<time_t, DataPoint> DataPointMap_t;
DataPointMap_t DataPointMap;

int first_esc_id = -1;

vector<string> ImportCSVData(std::string filename)
{
   vector<string> words;

   std::ifstream file( filename );
   std::stringstream str_strm;
   if ( file )
   {
      str_strm << file.rdbuf();
      file.close();
   }
   else
   {
      std::cout<<"error";
      return words;
   }
   
   std::string tmp;
   while (std::getline(str_strm, tmp)) {
      // Provide proper checks here for tmp like if empty
      // Also strip down symbols like !, ., ?, etc.
      // Finally push it.
      words.push_back(tmp);
   }

   return words;
}

void InsertDataPointFromLine( std::string input, DataPointMap_t* datapointMap )
{
   time_t inputTime;

   char delim = ','; // Ddefine the delimiter to split by
   std::string tmp;
   int index = 0;
   std::string::size_type sz;

   DataPoint datapoint;

   if( input.find("header") != std::string::npos ) //ignore line cause it is header data
   {   
      return;
   }
   else if( input.find("gps") != string::npos ) 
   {
      //is gps data
      istringstream s( input );
      while( getline( s, tmp, delim ) ) 
      {
         switch( index++ )
         {
            case 0: //time_t
            {
               struct tm tm;
               strptime(tmp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
               inputTime = mktime(&tm);  // t is now your desired time_t
            }
            break;
            case 2: //satellites
               datapoint.satellites = stoi( tmp );
            break;
            case 3: //altitude
               datapoint.altitude = stod( tmp );
            break;
            case 4: //speed
               datapoint.speedGPS = stod( tmp );
            break;
            case 5: //latitude
               datapoint.lat = stod( tmp );
            break;
            case 6: //longitude
               datapoint.lon = stod( tmp );
            break;
            default:
            break;
         }
      }
      
      if( datapointMap->count( inputTime ) > 0 )
      {
         (*datapointMap)[ inputTime ].satellites   = datapoint.satellites;
         (*datapointMap)[ inputTime ].altitude     = datapoint.altitude;
         (*datapointMap)[ inputTime ].speedGPS     = datapoint.speedGPS;
         (*datapointMap)[ inputTime ].lat          = datapoint.lat;
         (*datapointMap)[ inputTime ].lon          = datapoint.lon;
      }
      else
      {
         datapointMap->insert( { inputTime, datapoint } );
      }
   }
   else if( input.find("esc") != string::npos) //is esc data
   {
      istringstream s( input );
      while( std::getline( s, tmp, delim ) ) 
      {
         switch( index++ )
         {
            case 0: //time_t
            {
               struct tm tm;
               strptime(tmp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
               inputTime = mktime(&tm);  // t is now your desired time_t
            }
            break;
            case 2: //esc_id
            {
               int this_esc_id = stoi(tmp);
               if (first_esc_id == -1)
               {
                  first_esc_id = this_esc_id;
               } 
               else if (this_esc_id != first_esc_id)
               {
                  //TODO: skipping record for now
                  return;
               }
            }
            break;
            case 3: //voltage
               datapoint.vin = stod( tmp );
            break;
            case 4: //motor_temp
               datapoint.tempMotor = stod( tmp );
            break;
            case 5: //esc_temp
               datapoint.tempESC = stod( tmp );
            break;
            case 6: //duty_cycle
               datapoint.dutyCycle = stod( tmp );
            break;
            case 7: //motor_current
               datapoint.currentMotor = stod( tmp );
            break;
            case 8: //battery_current
               datapoint.currentBattery = stod( tmp );
            break;
            case 9: //watt_hours
               datapoint.wattHours = stod( tmp );
            break;
            case 10: //watt_hours_regen
               datapoint.wattHoursRegen = stod( tmp );
            break;
            case 11: //e_rpm
               datapoint.eRPM = stoi( tmp );
            break;
            case 12: //e_distance
               datapoint.eDistance = stoi( tmp );
            break;
            case 13: //fault
               datapoint.fault = stoi( tmp );
            break;
            case 14: //speed_kph
               datapoint.speed = stod( tmp );
            break;
            case 15: //distance_km
               datapoint.distance = stod( tmp );
            break;
            default:
            break;
         }
      }

      if( datapointMap->count( inputTime ) > 0 )
      {
         (*datapointMap)[ inputTime ].vin             = datapoint.vin;
         (*datapointMap)[ inputTime ].tempMotor       = datapoint.tempMotor;
         (*datapointMap)[ inputTime ].tempESC         = datapoint.tempESC;
         (*datapointMap)[ inputTime ].dutyCycle       = datapoint.dutyCycle;
         (*datapointMap)[ inputTime ].currentMotor    = datapoint.currentMotor;
         (*datapointMap)[ inputTime ].currentBattery  = datapoint.currentBattery;
         (*datapointMap)[ inputTime ].wattHours       = datapoint.wattHours;
         (*datapointMap)[ inputTime ].wattHoursRegen  = datapoint.wattHoursRegen;
         (*datapointMap)[ inputTime ].eRPM            = datapoint.eRPM;
         (*datapointMap)[ inputTime ].eDistance       = datapoint.eDistance;
         (*datapointMap)[ inputTime ].fault           = datapoint.fault;
         (*datapointMap)[ inputTime ].speed           = datapoint.speed;
         (*datapointMap)[ inputTime ].distance        = datapoint.distance;
      }
      else
      {
         datapointMap->insert( { inputTime, datapoint } );
      }
   }
}

int EncodeActivityFile(std::string filename)
{
   // Open the file
   std::fstream file;
   filename.append(".fit");
   file.open( filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

   if (!file.is_open())
   {
      printf("Error opening file ExampleActivity.fit\n");
      return -1;
   }

   // Create a FIT Encode object
   fit::Encode encode(fit::ProtocolVersion::V20);

   // Write the FIT header to the output stream
   encode.Open(file);

   // The starting timestamp for the activity
   fit::DateTime startTime(DataPointMap.begin()->first);
   double startWattHours = DataPointMap.begin()->second.wattHours;
   double startWattHoursRegen = DataPointMap.begin()->second.wattHoursRegen;
   double startDistance = DataPointMap.begin()->second.distance;
   

   // Every FIT file MUST contain a File ID message
   fit::FileIdMesg fileIdMesg;
   fileIdMesg.SetType(FIT_FILE_ACTIVITY);
   fileIdMesg.SetManufacturer(FIT_MANUFACTURER_DEVELOPMENT);
   fileIdMesg.SetProduct(1);
   fileIdMesg.SetTimeCreated(startTime.GetTimeStamp());
   // You should create a serial number unique to your platform
   srand((unsigned int)time(NULL));
   fileIdMesg.SetSerialNumber(rand() % 10000 + 1);
   encode.Write(fileIdMesg);

   // Create the Developer Id message for the developer data fields.
   fit::DeveloperDataIdMesg developerIdMesg;
   // It is a BEST PRACTICE to use the same Guid for all FIT files created by your platform
   // 00010203-0405-0607-0809-0A0B0C0D0E0F
   FIT_UINT8 appId[] = { 0x03,0x02,0x01,0x00,0x05,0x04,0x07,0x06,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };
   for (FIT_UINT8 i = 0; i < 16; i++)
   {
      developerIdMesg.SetApplicationId(i, appId[i]);
   }
   developerIdMesg.SetDeveloperDataIndex(0);
   developerIdMesg.SetApplicationVersion(110);
   encode.Write(developerIdMesg);

   // Timer Events are a BEST PRACTICE for FIT ACTIVITY files
   fit::EventMesg eventMesgStart;
   eventMesgStart.SetTimestamp(startTime.GetTimeStamp());
   eventMesgStart.SetEvent(FIT_EVENT_TIMER);
   eventMesgStart.SetEventType(FIT_EVENT_TYPE_START);
   encode.Write(eventMesgStart);

   // Every FIT ACTIVITY file MUST contain Record messages
   fit::DateTime timestamp(startTime);

// Convert DataPointMap to FIT records
   double gpsscalar = (double)( pow( 2, 31 ) / 180 );


   for (DataPointMap_t::iterator it=DataPointMap.begin(); it!=DataPointMap.end(); ++it)
   {
      timestamp = fit::DateTime(it->first);

      DataPoint datapoint = it->second;

      // Create a new Record message and set the timestamp
      fit::RecordMesg recordMesg;
      recordMesg.SetTimestamp( timestamp.GetTimeStamp() );
      recordMesg.SetTemperature( datapoint.tempESC ); //ESC Temperature
      recordMesg.SetMotorPower( ceil( datapoint.vin * datapoint.currentMotor ) ); //Watts

      time_t time = timestamp.GetTimeT() - startTime.GetTimeT();
      double hours = (double)time / 60.0 / 60.0;
      //recordMesg.SetAccumulatedPower( ((datapoint.wattHours - startWattHours) - (datapoint.wattHoursRegen - startWattHoursRegen)) * hours );  //Watt_hours - watt_hours_regen
      recordMesg.SetAccumulatedPower( (datapoint.wattHours - startWattHours) * hours );  //Watt_hours / time
      //TODO: variable control over ESC/GPS speed
      recordMesg.SetSpeed( fabs( datapoint.speedGPS * 0.277778 ) ); //Meters/sec
      recordMesg.SetDistance( (datapoint.distance - startDistance) * 100 ); //Meters
      recordMesg.SetGpsAccuracy( datapoint.satellites ); //Satellites
      recordMesg.SetAltitude( datapoint.altitude );
      recordMesg.SetEnhancedSpeed( fabs( datapoint.speedGPS * 0.277778 ) ); // Meters/sec
      recordMesg.SetPositionLat( gpsscalar * datapoint.lat );
      recordMesg.SetPositionLong( gpsscalar * datapoint.lon );

      //recordMesg.SetBatterySoc(); // battery percentage
      if (datapoint.satellites > 0)
      {
         // Write the Rercord message to the output stream
         encode.Write(recordMesg);
         
         fit::DeviceInfoMesg deviceInfoMesg;
         deviceInfoMesg.SetTimestamp(timestamp.GetTimeStamp());
         deviceInfoMesg.SetBatteryVoltage(it->second.vin);

         // Write the DeviceInfo message to the output stream
         encode.Write(deviceInfoMesg);
      }
   }

   // Timer Events are a BEST PRACTICE for FIT ACTIVITY files
   fit::EventMesg eventMesgStop;
   eventMesgStop.SetTimestamp(timestamp.GetTimeStamp());
   eventMesgStop.SetEvent(FIT_EVENT_TIMER);
   eventMesgStop.SetEventType(FIT_EVENT_TYPE_STOP);
   encode.Write(eventMesgStop);

   // Every FIT ACTIVITY file MUST contain at least one Lap message
   fit::LapMesg lapMesg;
   lapMesg.SetTimestamp(timestamp.GetTimeStamp());
   lapMesg.SetStartTime(startTime.GetTimeStamp());
   lapMesg.SetTotalElapsedTime((FIT_FLOAT32)(timestamp.GetTimeStamp() - startTime.GetTimeStamp()));
   lapMesg.SetTotalTimerTime((FIT_FLOAT32)(timestamp.GetTimeStamp() - startTime.GetTimeStamp()));
   encode.Write(lapMesg);

   // Every FIT ACTIVITY file MUST contain at least one Session message
   fit::SessionMesg sessionMesg;
   sessionMesg.SetTimestamp(timestamp.GetTimeStamp());
   sessionMesg.SetStartTime(startTime.GetTimeStamp());
   sessionMesg.SetTotalElapsedTime((FIT_FLOAT32)(timestamp.GetTimeStamp() - startTime.GetTimeStamp()));
   sessionMesg.SetTotalTimerTime((FIT_FLOAT32)(timestamp.GetTimeStamp() - startTime.GetTimeStamp()));
   sessionMesg.SetSport(FIT_SPORT_GENERIC);
   sessionMesg.SetSubSport(FIT_SUB_SPORT_GENERIC);
   sessionMesg.SetFirstLapIndex(0);
   sessionMesg.SetNumLaps(0);

   encode.Write(sessionMesg);
   
   // Every FIT ACTIVITY file MUST contain EXACTLY one Activity message
   fit::ActivityMesg activityMesg;
   activityMesg.SetTimestamp(timestamp.GetTimeStamp());
   activityMesg.SetNumSessions(1);
   //int timezoneOffset = -7 * 3600;
   //activityMesg.SetLocalTimestamp((FIT_LOCAL_DATE_TIME)((int)timestamp.GetTimeStamp() + timezoneOffset));
   encode.Write(activityMesg);

   // Update the data size in the header and calculate the CRC
   if (!encode.Close())
   {
      printf("Error closing encode.\n");
      return -1;
   }

   // Close the file
   file.close();

   return 0;
}

int main(int argc, char **argv)
{
   if( argc != 2 )
   {
      cout << "add file argument pls" << endl;
      return 0;
   }

   cout << "Starting encode" << endl;
   auto lines = ImportCSVData( std::string( argv[ 1 ] ) );

   for( int i=0; i< lines.size(); ++i)
   {
      InsertDataPointFromLine( lines.at( i ), &DataPointMap );
   }

   DataPointMap.erase(prev(DataPointMap.end()));

   std::cout << DataPointMap.size() << endl;

   EncodeActivityFile( std::string( argv[ 1 ] ) );

   return 0;
}
