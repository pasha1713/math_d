#include <nmea/nmea.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#define NUM_POINTS 5000000
#include "progress_bar.hpp"
//#include <iostream>
//#include <thread>
//#include <chrono>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
using namespace boost;
using namespace std;
#include <iostream>
#include <exception>
#ifdef NMEA_WIN
#   include <io.h>
#endif

void trace(const char *str, int str_size)
{
	printf("Trace: ");
	write(1, str, str_size);
	printf("\n");
}
void error(const char *str, int str_size)
{
	printf("Error: ");
	write(1, str, str_size);
	printf("\n");
}
nmeaPOS pos_[NUM_POINTS], pos_moved[NUM_POINTS][2], camera;
double dist[NUM_POINTS][3];
double azimuth[NUM_POINTS][2], azimuth_moved[NUM_POINTS];
nmeaTIME utc_it[NUM_POINTS], YYYYDDMM;
double speed[NUM_POINTS];
time_t delta_time_sec[NUM_POINTS];

// Below function extracts characters present in src
// between m and n (excluding n) 
char* substr(const char *src, int m, int n)
{
	// get length of the destination string
	int len = n - m;

	// allocate (len + 1) chars for destination (+1 for extra null character)
	char *dest = (char*)malloc(sizeof(char) * (len + 1));

	// extracts characters between m'th and n'th index from source string
	// and copy them into the destination string
	for (int i = m; i < n && (*(src + i) != '\0'); i++)
	{
		*dest = *(src + i);
		dest++;
	}

	// null-terminate the destination string
	*dest = '\0';

	// return the destination string
	return dest - len;
}
int main()
{
	try
	{
		int it;
		FILE *in_file, *out_file;
		//const char *in_path = "d:\\gps\\in_msk\\full\\";
		//const char *in_file_mask = "2020*.log";
		char in_file_mask_full[2048];
		char in_file_full[2048];
		//char *out_file_full = "rez.txt";
		//char in_file[256];
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;
		char buff[2048];
		int size;
		int max_points;
		double camera_m, camera_lat_gradus, camera_lon_gradus;
		int camera_it = 0;
		struct tm camera_time, camera_time_start, camera_time_stop;
		time_t time_sec, start_time, stop_time, delta_time = 60;	//sec 
		//unsigned camera_max_distance = 90; //m
		//unsigned speed_limit = 100; // km/hr
		nmeaINFO info, info_camera_m;
		int file_count = 0, count = 0;
		string file_in_str;
		using boost::lexical_cast; // needed to cast parsed INI strings to integer type
		string in_path_str, in_file_mask_str, out_file_full_str;

		boost::property_tree::ptree pt; // // Create an empty property tree object
		boost::property_tree::ini_parser::read_ini("input.ini", pt); // read example ini file

		delta_time = (time_t)(lexical_cast<int>(pt.get<std::string>("limits.delta_time"))); // in section "box" get key "limits"
		unsigned camera_max_distance = lexical_cast<unsigned>(pt.get<std::string>("limits.camera_max_distance"));
		unsigned speed_limit = lexical_cast<unsigned>(pt.get<std::string>("limits.speed_limit"));

		in_path_str = pt.get<std::string>("files.in_path");
		const char * in_path = in_path_str.c_str();
		in_file_mask_str = pt.get<std::string>("files.in_file_mask");
		const char * in_file_mask = in_file_mask_str.c_str();
		out_file_full_str = pt.get<std::string>("files.out_file_full");
		const char * out_file_full = out_file_full_str.c_str();

		camera_lat_gradus = lexical_cast<double>(pt.get<std::string>("camera.lat"));
		camera_lon_gradus = lexical_cast<double>(pt.get<std::string>("camera.lon"));

		// and assign the rest of your variables...
		printf("delta_time=%lld, camera_max_distance=%d, speed_limit=%d\nin_path=%s, in_file_mask=%s, out_file_full=%s\n",
			delta_time, camera_max_distance, speed_limit, in_path, in_file_mask, out_file_full);
		printf("camera_lat_gradus=%10.6lf, camera_lon_gradus=%10.6lf\n", camera_lat_gradus, camera_lon_gradus);

		//camera.lat = nmea_degree2radian(55.479265);
		//camera.lon = nmea_degree2radian(36.94068);
		camera.lat = nmea_degree2radian(camera_lat_gradus);
		camera.lon = nmea_degree2radian(camera_lon_gradus);
		sprintf(in_file_mask_full, "%s%s", in_path, in_file_mask);
		hFind = FindFirstFile(in_file_mask_full, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			printf("FindFirstFile failed (%d)\n", GetLastError());
			return -1;
		}
		do 
			file_count++;
		while (FindNextFile(hFind, &FindFileData) != 0);
	
		ProgressBar *bar1 = new ProgressBar(file_count, "Processing");

		hFind = FindFirstFile(in_file_mask_full, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			printf("FindFirstFile failed (%d)\n", GetLastError());
			return -1;
		}
		out_file = fopen(out_file_full, "w");
		if (!out_file) return -1;
		do
		{
			camera_m = 99999999;
			it = 0;

			YYYYDDMM.year = atoi(substr(FindFileData.cFileName, 0, 4)) - 1900;
			YYYYDDMM.mon  = atoi(substr(FindFileData.cFileName, 4, 6)) - 1;
			YYYYDDMM.day  = atoi(substr(FindFileData.cFileName, 6, 8));
		
			sprintf(in_file_full, "%s%s", in_path, FindFileData.cFileName);
			in_file = fopen(in_file_full, "rb");
	
			if (!in_file)  return -1;
	
			//	nmea_property()->trace_func = &trace;
			nmea_property()->error_func = &error;

			nmeaPARSER parser;
			nmea_parser_init(&parser);
			nmea_zero_INFO(&info_camera_m);
			while (!feof(in_file))
			{
				nmea_zero_INFO(&info);
				size = (int)fread(&buff[0], 1, 100, in_file);
				//fgets(&buff[0], 100, in_file);
				//size = strlen(buff) - 1;
				nmea_parse(&parser, &buff[0], size, &info);
				nmea_info2pos(&info, &pos_[it]);
				utc_it[it] = info.utc;
				if (it)
				{
					dist[it][1] = nmea_distance(&pos_[it - 1], &pos_[it]); // distance by 1s
					camera_time.tm_sec = utc_it[it - 1].sec;   // seconds after the minute - [0, 60] including leap second
					camera_time.tm_min = utc_it[it - 1].min;   // minutes after the hour - [0, 59]
					camera_time.tm_hour = utc_it[it - 1].hour;  // hours since midnight - [0, 23]
					camera_time.tm_mday = YYYYDDMM.day;   // day of the month - [1, 31]
					camera_time.tm_mon = YYYYDDMM.mon;   // months since January - [0, 11]
					camera_time.tm_year = YYYYDDMM.year;  // years since 1900
					start_time = mktime(&camera_time);
					camera_time.tm_sec = utc_it[it].sec;   // seconds after the minute - [0, 60] including leap second
					camera_time.tm_min = utc_it[it].min;   // minutes after the hour - [0, 59]
					camera_time.tm_hour = utc_it[it].hour;  // hours since midnight - [0, 23]
					stop_time = mktime(&camera_time);
					delta_time_sec[it] = stop_time - start_time;
					speed[it] = dist[it][1] / delta_time_sec[it] * 3.6; // km/hr
					info.speed = speed[it];
				}
				if ((pos_[it].lat != 0) && (pos_[it].lon != 0))
					if ((pos_[it].lat != pos_[it - 1].lat) || (pos_[it].lon != pos_[it - 1].lon))
					{
						dist[it][0] = nmea_distance(&camera, &pos_[it]); // distance from camera
						if (camera_m > dist[it][0])
						{
							camera_m = dist[it][0];
							camera_it = it;
							info_camera_m = info; // save info for distance camera mininum
						}
						it++;
					}
			}
			fclose(in_file);

			bar1->Progressed(++count);

			max_points = it;
			nmea_parser_destroy(&parser);
			/*
			for (it = 0; it < max_points; ++it)
			{
				dist[it][2] = nmea_distance(&camera, &pos_[it]); // distance from camera
				if (camera_m > dist[it][2])
				{
					camera_m = dist[it][2];
					camera_it = it;
				}
			}*/
			camera_time.tm_sec = utc_it[camera_it].sec;   // seconds after the minute - [0, 60] including leap second
			camera_time.tm_min = utc_it[camera_it].min;   // minutes after the hour - [0, 59]
			camera_time.tm_hour = utc_it[camera_it].hour;  // hours since midnight - [0, 23]
			camera_time.tm_mday = YYYYDDMM.day;   // day of the month - [1, 31]
			camera_time.tm_mon = YYYYDDMM.mon;   // months since January - [0, 11]
			camera_time.tm_year = YYYYDDMM.year;  // years since 1900
			time_sec = mktime(&camera_time);
			start_time = time_sec - delta_time;
			stop_time = time_sec + delta_time;
			camera_time_start = *localtime(&start_time);
			camera_time_stop = *localtime(&stop_time);
			time_sec += 3 * 3600;
			camera_time = *localtime(&time_sec);

			/* Output of results */
	
			if (dist[camera_it][0] < camera_max_distance)
			{
				if (speed[camera_it] < speed_limit) fprintf(out_file, "rem ");
				fprintf(out_file, 
					"call go.cmd list %s %02d:%02d:%02d %04d%02d%02d%02d%02d%02d %04d%02d%02d%02d%02d%02d %5.1lf %5.1lf %10.lfm",
					FindFileData.cFileName, camera_time.tm_hour, camera_time.tm_min, camera_time.tm_sec,
					camera_time_start.tm_year + 1900, camera_time_start.tm_mon + 1, camera_time_start.tm_mday,
					camera_time_start.tm_hour, camera_time_start.tm_min, camera_time_start.tm_sec,
					camera_time_stop.tm_year + 1900, camera_time_stop.tm_mon + 1, camera_time_stop.tm_mday,
					camera_time_stop.tm_hour, camera_time_stop.tm_min, camera_time_stop.tm_sec, 
					speed[camera_it], dist[camera_it][1], dist[camera_it][0]);
				fprintf(out_file, "\n");
				fflush(out_file);
			}
		}
		while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
		fclose(out_file);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
