#include "opencv2/opencv_modules.hpp"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <map>
#include <time.h>

#define MAX_DISTANCE 1

#ifndef HAVE_OPENCV_NONFREE

int main(int, char**)
{
	printf("The sample requires nonfree module that is not available in your OpenCV distribution.\n");
	return -1;
}

#else

# include "opencv2/core/core.hpp"
# include "opencv2/features2d/features2d.hpp"
# include "opencv2/highgui/highgui.hpp"
# include "opencv2/nonfree/features2d.hpp"

using namespace cv;
using namespace std;

void readme();

/**
 * @function main
 * @brief Main function
 */
int main( int argc, char** argv )
{
	if( argc != 1 ){
		readme();
		return -1;
	}

	string dir_source("/home/wangd7/Desktop/flannMatcher/targets");
	string dir_target("/home/wangd7/Desktop/flannMatcher/source");
	string fp_source, fp_target;
	DIR *dp_source, *dp_target;
	struct dirent *dirp_source, *dirp_target;

	double min_distance;
	string str_source;
	map<string, string> answer;
	vector<string> filename;

	dp_target = opendir(dir_target.c_str());

	if( dp_target == NULL){
		cout<<"Error opening "<<endl;
		return 0;
	}

	// cnt is used as the progress indicator
	int cnt = 0;

	// time counter
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	while((dirp_target = readdir(dp_target))){
		min_distance = MAX_DISTANCE;
		str_source.clear();

		fp_target = dir_target + "/" + dirp_target->d_name;

		if(strcmp(dirp_target->d_name, ".") == 0 || strcmp(dirp_target->d_name, "..") == 0 ){
			continue;
		}else{
			cout<<"Start processing " <<++cnt<<" sub image ......"<<endl;

			//cout<<"target file is "<<fp_target<<endl;
			Mat img_2 = imread( fp_target, CV_LOAD_IMAGE_GRAYSCALE );
			if( !img_2.data ){
				printf(" --(!) Error reading images \n");
				return -1;
			}

			dp_source = opendir(dir_source.c_str());
			if( dp_source == NULL){
				cout<<"Error opening "<<endl;
				return 0;
			}

			while((dirp_source = readdir(dp_source))){
				fp_source = dir_source + "/" + dirp_source->d_name;
				if(strcmp(dirp_source->d_name, ".") == 0 || strcmp(dirp_source->d_name, "..") == 0 ){
					continue;
				}
				else{
					if( std::find(filename.begin(), filename.end(), dirp_source->d_name) != filename.end() )
						continue;

					//cout<<"source file is "<<fp_source<<endl;
					Mat img_1 = imread( fp_source.c_str(), CV_LOAD_IMAGE_GRAYSCALE );

					if( !img_1.data ){
						printf(" --(!) Error reading images \n");
						return -1;
					}

					//-- Step 1: Detect the keypoints using SURF Detector
					int minHessian = 1000;
					SurfFeatureDetector detector( minHessian );
					std::vector<KeyPoint> keypoints_1, keypoints_2;
					detector.detect( img_1, keypoints_1 );
					detector.detect( img_2, keypoints_2 );

					//-- Step 2: Calculate descriptors (feature vectors)
					SurfDescriptorExtractor extractor;
					Mat descriptors_1, descriptors_2;
					extractor.compute( img_1, keypoints_1, descriptors_1 );
					extractor.compute( img_2, keypoints_2, descriptors_2 );

					//-- Step 3: Matching descriptor vectors using FLANN matcher
					FlannBasedMatcher matcher;
					std::vector< DMatch > matches;
					matcher.match( descriptors_1, descriptors_2, matches );

					/*** parallel part beg ***/
					//matcher.knnMatch(descriptors_1, descriptors_2, );
					/*** parallel part end ***/

					double max_dist = 0; double min_dist = 100;

					//-- Quick calculation of max and min distances between keypoints
					for( int i = 0; i < descriptors_1.rows; i++ ){
						double dist = matches[i].distance;
						if( dist < min_dist )
							min_dist = dist;
						if( dist > max_dist )
							max_dist = dist;
					}

					// printf("-- Max dist : %f \n", max_dist );
					// printf("-- Min dist : %f \n", min_dist );

					//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
					//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
					//-- small)
					//-- PS.- radiusMatch can also be used here.
					std::vector< DMatch > good_matches;
					for( int i = 0; i < descriptors_1.rows; i++ ){
						if( matches[i].distance <= max(2*min_dist, 0.02) ){
							good_matches.push_back( matches[i]);
						}
					}

					//-- Draw only "good" matches
					//Mat img_matches;
					// drawMatches( img_1, keypoints_1, img_2, keypoints_2,
					//         good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
					//         vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

					//-- Show detected matches
					//imshow( "Good Matches", img_matches );

					double sum = 0;
					double ave_sum = 0;
					for( int i = 0; i < (int)good_matches.size(); i++ ){
						sum += good_matches[i].distance;
						// printf( "-- Good Match [%d] Keypoint 1: %4d  -- Keypoint 2: %4d  good distance: %.4f\n",
						//         i, good_matches[i].queryIdx, good_matches[i].trainIdx, good_matches[i].distance );
					} ave_sum = sum/good_matches.size();
					if(ave_sum < min_distance)
					{
						min_distance = ave_sum;
						str_source = dirp_source->d_name;
					}
				}
			}

			filename.push_back(str_source);
			string src_name, tar_name;
			istringstream iss_tar(dirp_target->d_name);
			getline(iss_tar, tar_name, '.');
			istringstream iss_src(str_source);
			getline(iss_src, src_name, '.');
			answer.insert(pair<string, string>(tar_name, src_name));
		}
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	uint64_t diff = (end.tv_sec-start.tv_sec)+end.tv_nsec-start.tv_nsec;
	printf("Elapsed process CPU time = %llu seconds \n", (long long unsigned int)diff);

	closedir(dp_source);
	closedir(dp_target);

	cout<<"{\"env\":\"dev\",\"answers\":{";
	int size = answer.size()-1;
	for(map<string, string>::iterator iter = answer.begin(); iter != answer.end(); iter++){
		cout<<"\"" << iter->first<< "\"" <<":"<<iter->second;
		if(size != 0)
			cout<<",";
		size--;
	}
	cout<<"}}"<<endl;
}

/**
 * @function readme
 */
void readme(){
	cout<<" Usage: ./SURF_FlannMatcher <img1> <img2>"<<endl;
}
#endif

