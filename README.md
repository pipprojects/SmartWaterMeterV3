# SmartWaterMeterV3
A water meter that logs data to a remote database using an API based on Frapi.
The mechanics and electrics of this project are identical to SmartWaterMeter (https://www.hackster.io/pipster-18/smart-water-meter-8c67bd) but the Photon code and backend are different. This project logs data to a database via an API taht is under your control, rather than to a 3rd party. This can be useful if you don't want to share your data or grow your own application. The database and API can nbe on different hosts, physical, virtual, in the server room or in the cloud.

I have taken inspiration from the way SugarCRM 7 is configured so if you are familiar with that you may find some of the following concepts familiar.

## FRAPI
The FRAPI code can be downloaded from the API folder. You will need to change some details based on your set-up, such as hostnames and database credentials.

FRAPI uses the idea of an admin host url and an API host url, so you need to configure your HTTP server and DNS to meet this criteria. You can use the example one for Apache in the conf directory under API. Extract the api cource code and point your http server at that location.

In this configuration, the API hostname is api.watermeter.local and the api admin hostname is apiadmin.watermeter.local. You will need a local DNS configuration to make this work, or you could put it all in the cloud and use a real domain name.

The username for the API admin page is "admin" and the password is "password".

## Database
I have used MySQL as the database. There is a sample one under the Database folder. You will need to create an empty database (create database watermeter;) and then restore using mysql (gunzip watermeter-20160330-122333.sql.gz; mysql -u user -p watermeter < watermeter-20160330-122333.sql).

There are 2 record tables
+ Devices
+ Events

The ids for each record are unique butnot auto incremented. This idea is similar to SugarCRM and makes deleting records via tha API much simpler since you do not have to (and it may not be appropriate to) delete dependent records. It does mean, however, that you may end up with orphaned records that do not relate to any existing record. This is up to you to manage as a clean-up process, or feel free to modify the code.

Records are not actually deleted when the DELETE action is run, Instead, the record is marked as deleted and an external program, purge-records.sh, is run by cron to clean up the records at a later date.

The `admin_` tables are for database administration. This is a work in progress and in the future, when you create a Collection or Resource in FRAPI, it will add database tables automatically. These admin tables will record this activity.

## API

There are 4 HTTP verbs that are used here.
+ GET
+ POST
+ PUT
+ DELETE

Depending on if using the verbs on a Collection (/collection) or Resource (/collection/:resourceid), GET and POST do slightly different things.

## Collection
There are 2 Collections in this watermeter applciation
+ Devices
+ Events

Devices operates on the database table "Devices".
+ A GET will retrieve all the entries in the Devices table but can be filtered with the "filter" list which, if supplied, will be run as a "WHERE" clause.
eg. `curl --digest -u "test:34c285b25ac62f9472265d1e41f8a77f5d2382f6" -X GET http://api.watermeter.local/devices -H 'Content-Type: application/json'`

+ A POST will add a new resource to Devices
eg. `curl --digest -u "test:34c285b25ac62f9472265d1e41f8a77f5d2382f6" -X POST http://api.watermeter.local/devices -d '{"id": "7f96af4aac49acfcb9b10198a2280665", "serial_number": "2",}'-H 'Content-Type: application/json'`

+ A PUT will not do anything

+ A DELETE will set the "deleted" field to true. Records are deleted when the purge-records.sh script is run, probably by cron

Similary, Events operates as above, but since Events can be related to Devices, retrieving an entry will not just return the ID of the Device but the Device Name as well.

## Resource
There are 2 Resources in the watermeter application
+ Device
+ Event

Acting on the resources will have the following effect

+ A GET will retrieve a single record with the specified ID
eg. `curl --digest -u "test:34c285b25ac62f9472265d1e41f8a77f5d2382f6" -X GET http://api.watermeter.local/devices/7f96af4aac49acfcb9b10198a2280665 -H 'Content-Type: application/json'`

+ A POST will retrieve a single record with the specified ID
eg. `curl --digest -u "test:34c285b25ac62f9472265d1e41f8a77f5d2382f6" -X POST http://api.watermeter.local/devices/7f96af4aac49acfcb9b10198a2280665 -H 'Content-Type: application/json'`

+ A PUT will modify an existing resource
eg. `curl --digest -u "test:34c285b25ac62f9472265d1e41f8a77f5d2382f6" -X PUT http://api.watermeter.local/devices/7f96af4aac49acfcb9b10198a2280665 -d '{"serial_number": "3",}'-H 'Content-Type: application/json'`

+ A DELETE will set the "deleted" field to true. Records are deleted when the purge-records.sh script is run, probably by cron

## Photon
FRAPI uses http digest authentication in the PHP application, not at the Apache web server (ie you do not use `mod_auth`). I assume this is so it is easy to make the API private or public. The username and password for a user who needs to acess the API is set in the admin interface (apiadmin.watermeter.local) under Partners.

The HttpClient library that can be found for the Photon does not do any kind of authentication so I have modified it to do http digest authentication only. This what the HttpClient2.cpp and HttpClient2.h files do. Http digest authentication requires an md5 library and some code to create the appropriate headers. These are the files md5.cpp, md5.h, digest.cpp, digest.h. I downloaded these and modified slightly to use with the Photon. The original licences are in the files.


# Set up the API host details

In the Photon folder there is the .ino file for the Photon. You will need to change the API access details

`  APIData.Host = "api.watermeter.localhost";
// IP Address of this host
  APIData.IP = IPAddress(192,168,0,1);
  APIData.Port = 80;
  APIData.Path="/events";
// This is set in the FRAPI admin interface under Partners
  APIData.Credentials.username = "test";
  APIData.Credentials.password = "34c285b25ac62f9472265d1e41f8a77f5d2382f6";
// Set this to whatever you want but should have an entry in the Devices table
  APIData.DeviceID = "7f96af4aac49acfcb9b10198a2280665";`


# Operation
When the Photon boots up, it will log this as a "Boot" event in the database. It will also log details of Free Memory as freemem reports it. This is just a little bit of debugging which you might find helpful. You can turn this off by commenting out the PhotonStatus thread.
When the watermeter or the switch is closed, a "Closed" event is logged. The multi-coloured LED will turm from yellow to blue. Once the thread has completed the call to the API, the LED will turn purple. When the watermeter or switch goes open, an "Open" event is logged and the LED turns green. Once the API call is completed, the LED turns back to yellow.
For all events, the date the even was created is recorded as well as a Unix style timestamp so you can easily determine what happened and when. A value is recorded for the "Closed" event of "10" which indicates that 10 litres has passed through the watermeter. However, if you analysing data externally, you just need the "Open" event timestamps and you can work out volume and cost.





