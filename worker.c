#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include "worker.h"

#define LINE_LENGTH 255

/*
 * Read an image from a file and create a corresponding 
 * image struct 
 */
 
 char H1;

Image* read_image(char *filename){	
	//Instatiate return image pointer	
	Image *img = malloc(sizeof(Image));
	//Instatiate file pointer
	FILE *image_file;
	//Open file for reading
	image_file = fopen(filename, "r");
	//Have an error checker for closing the file
	int error;
	//Instatiate elements
	int width, height, max_value;
	//Read image file and store the header values
	fscanf(image_file, "%s %d %d %d", &H1, &width, &height, &max_value);
	//Check if magic number == P3
	if (strcmp(&H1, "P3") != 0){
		return NULL;
	}
	//Assign header values to fscanf's stored variables
	img->width = width;
	img->height = height;
	img->max_value = max_value;
	//Instatiate a pixel array
	Pixel *pixels = malloc((width*height) * sizeof(Pixel));
	//Instatiate current pixels values
	int curr_r, curr_g, curr_b;
	//Instatiate Pixel
	Pixel curr_P;
	//Keep reading width*height many times to store pixels
    for (int i = 0; i < width*height; i++) {
		//Scan each pixel element
		fscanf(image_file, "%d%d%d", &curr_r, &curr_g, &curr_b);
		//Store pixel elements to the current Pixel
		curr_P.red = curr_r;
		curr_P.green = curr_g;
		curr_P.blue = curr_b;
		//Add the current pixel to pixel array
		pixels[i] = curr_P;
	}
	//Assign the pixels array element to the pixel array we just created and loaded
	img->p = pixels;
	//Close the image file after finishing reading
	error = fclose(image_file);
    if (error != 0) {
        fprintf(stderr, "fclose failed\n");
        return NULL;
    }
	//Return image pointer
    return img;
}

/*
 * Print an image based on the provided Image struct 
 */

void print_image(Image *img){
        printf("P3\n");
        printf("%d %d\n", img->width, img->height);
        printf("%d\n", img->max_value);
       
        for(int i=0; i<img->width*img->height; i++)
           printf("%d %d %d  ", img->p[i].red, img->p[i].green, img->p[i].blue);
        printf("\n");
}

/*
 * Compute the Euclidian distance between two pixels 
 */
float eucl_distance (Pixel p1, Pixel p2) {
        return sqrt( pow(p1.red - p2.red,2 ) + pow( p1.blue - p2.blue, 2) + pow(p1.green - p2.green, 2));
}

/*
 * Compute the average Euclidian distance between the pixels 
 * in the image provided by img1 and the image contained in
 * the file filename
 */

float compare_images(Image *img1, char *filename) {
	//Use the task 1 helper function to make and image from a file pointer, and save that to an image pointer
    Image *img2 = read_image(filename);
	//Save the dimensions of the 2 images to compare later
	int img1_dim = ((*img1).width)*((*img1).height);
	int img2_dim = ((*img2).width)*((*img2).height);
	//Check if the dimensions of the 2 images are different
	if (img1_dim != img2_dim){
		//Return the max float value
		return FLT_MAX;
	}
	else {
		//Store the total eucl_distance as well as the average
		float total_dist = 0;
		float total_avg;
		//Iterate through all the pixels since in this case they have the same dimensions
		for (int i = 0; i < img1_dim; i++){
			//Add to the total dist the eucl dist of the ith pixel in both image's array list
			total_dist += eucl_distance(((*img1).p)[i], ((*img2).p)[i]);
		}
		//Calculate the average from the generated total distance
		total_avg = (total_dist)/img1_dim;
		//Return the total average
		return total_avg;
	}
}

/* process all files in one directory and find most similar image among them
* - open the directory and find all files in it 
* - for each file read the image in it 
* - compare the image read to the image passed as parameter 
* - keep track of the image that is most similar 
* - write a struct CompRecord with the info for the most similar image to out_fd
*/
CompRecord process_dir(char *dirname, Image *img, int out_fd){
	//Create a directory object
	DIR *dir;
	//Instatiate a directory entry
	struct dirent *ent;
	//Instatiate the return value CompRecord to have the max float value
	CompRecord Crec;
	CompRecord CurrRecord;
	Crec.distance = FLT_MAX;
	//Instatiate the current difference float to be used in file iteration
	float curr_diff;
	//Create a curr file instance
	char curr_file[PATHLENGTH];
	
	//Check if the directory can be opened
	if ((dir = opendir(dirname)) != NULL) {
		//Iterate through all files in the directory and store each file name in the file_names array
		while ((ent = readdir(dir)) != NULL) {
			if(strcmp(ent->d_name, ".") == 0 || 
				strcmp(ent->d_name, "..") == 0 ||
				strcmp(ent->d_name, ".svn") == 0){
					continue;
			}
			strcpy(curr_file, dirname);
			strncat(curr_file, "/", PATHLENGTH - strlen(curr_file) - 1);
			strncat(curr_file, ent->d_name, PATHLENGTH - strlen(curr_file) - 1);
			struct stat sbuf;
			if(stat(curr_file, &sbuf) == -1) {
				//This should only fail if we got the path wrong
				// or we don't have permissions on this entry.
				perror("stat");
				exit(1);
			} 
			if(S_ISDIR(sbuf.st_mode)) {
				if (out_fd != STDOUT_FILENO){
					CurrRecord = process_dir(curr_file, img, out_fd);
					if (CurrRecord.distance < Crec.distance){
						Crec.distance = CurrRecord.distance;
						strcpy(Crec.filename, CurrRecord.filename);
					}
				}
				
			}	
			else{
				curr_diff = compare_images(img, curr_file);
				//Check if the current difference is smaller than the CompRecords' current shortest
				if (Crec.distance > curr_diff){
					//Update the return CompRecord object's elements to the current file and distance
					Crec.distance = curr_diff;
					strcpy(Crec.filename, curr_file);
				}
			}
		}
		//Close the opened directory
		closedir (dir);
	}
	//If the directory can't be opened, error check
	else {
		perror ("");
		return Crec;
	}
	write(out_fd, Crec, sizeof(Crec));
	return Crec;
}




























