#include "StatsGenerator.h"

#include <iostream>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <chrono>
#include <ctime>

using namespace cv;
using namespace std;


StatsGenerator::StatsGenerator() :
	//frames(1),
	savable(false)
{
	AlgStartTime = chrono::system_clock::now();
}

StatsGenerator::StatsGenerator(std::string file_name) :
	savable(false)
{
    AlgStartTime = chrono::system_clock::now();
	
	set_save_file(file_name);
}

MovementSample StatsGenerator::create_sample(HandInfo info, bool save)
{
   int head_thresh = 100;   // Threshold for hands above head
   MovementSample sample;   // TIME, Velocity, Frequency                         
   Point center(0,0);       // Center point of hands                        
                                                                            
   center.y = (info.left_hand.center.y + info.right_hand.center.y) / 2;     // Averaged y hand center mid point
   center.x = (info.left_hand.center.x + info.right_hand.center.x) / 2;     // Averaged x hand center mid ponit 
                                                                            
   if (center.y < info.head.center.y + head_thresh)                         // If hands are above head + threshold 
   {                                                                        // Collected Data
       sample = oscillation_detection(sample, center, lastCenter, save);    // Calc frequency and velocity
       lastCenter = center;                                                 // Initialize for next cycle
       return sample;
   }
   else
   {
       drop = true;           // Drop frame if hands are above center of head
       sample.velocity = 0;   
       sample.frequency = 0;
       sample.time = std::chrono::system_clock::duration::zero();
       return sample;
   }                                                                                                             
}

MovementSample StatsGenerator::oscillation_detection(MovementSample sample, Point point, Point lastPoint, bool save)
{
   std::chrono::duration<double> timeDiff;
   displacement = point.y - lastPoint.y; 
   if (State == 1) // UP
   {
       if (displacement > 0) // Moving DOWN
       {
          if ((min_height - max_height) > dist_thresh)   // Distance travelled is larger than threshold
          { 
             timeDiff = chrono::system_clock::now() - start;   // Calculate duration of cycle
             start = chrono::system_clock::now();              // Reset start of cycle
             if (drop)
             {
                drop = false;   // Clear drop flag 
             }
             else
             { 
				sample.frequency = 1 / timeDiff.count();
				displacement = 2 * min_height - max_height + lastPoint.y;
				sample.velocity = displacement / timeDiff.count();
				sample.time = chrono::system_clock::now() - AlgStartTime;
				decay_time = timeDiff + sample.time;                
             }
             
			 State = 0;              // Transition state to moving DOWN
          }
          max_height = lastPoint.y;           
       }
   }
   else
   {
      if (displacement < 0)		     // If moving UP
      { 
	     min_height = lastPoint.y;   // Found a minimum point
	     State = 1;                  // Transition to moving UP
      }
   }
   if (sample.time > decay_time)
   {
      sample.frequency = 0;
      sample.velocity = 0;
      sample.time = chrono::system_clock::duration::zero();
   }
   // Output Metrics 
   if(save)
   {
      save_sample(sample);
   }
   return sample;
      
}

void StatsGenerator::save_sample(MovementSample sample)
{
	ofstream outdata;   //write variable

	outdata.open(file_path, ofstream::app);                        //open the file to write to    
	if (!outdata.is_open())                                        // if not success, throw exception
	{
		throw new std::runtime_error("Cannot open file: " + file_path);
	}
	if ((sample.velocity > 0) & (sample.frequency > 0) & (sample.time.count() > 0))
		outdata << sample.velocity << "," << sample.frequency << "," << sample.time.count() << endl;   // output metrics as "velocity" "frequency" "Time since Algorithm start" in adjacent cells.
	outdata.close();   // Close CSV file    
}

void StatsGenerator::set_save_file(std::string filename)
{
	file_path = filename;

	ofstream outdata;                             //write variable
	outdata.open(file_path, ofstream::out);      // Clear CSV file on first frame
	if (!outdata.is_open())                       // if not success, throw exception
	{
		throw new std::runtime_error("Cannot open file: " + file_path);
	}
	outdata.close();
	savable = true;
}

std::string StatsGenerator::get_save_file()
{
	return file_path;
}
