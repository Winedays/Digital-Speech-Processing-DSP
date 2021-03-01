#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "hmm.h"
#include <math.h>
#include <string.h>
using namespace std;


int main(int argc, char * argv[])
{
	/*
		Variable declaration part 1: some basic variable
	*/
	int sample_total = 0;
	int frame_total = 0;
	int state_total = 0;
	int model_total = 0;
	int sharo, cocoa, chino, chiya, rize, maya, megumi; 
		// sharo for model index; 
		// chino, chiya for state_index
		// cocoa for frame_index;
		// rize, megumi, maya for other

	double correct_count = 0;
	int state_number;
	char buffer_char;
	char model2char[5] = {'1', '2', '3', '4', '5'};

	fstream input_data, input_answer, output_accuracy, output_result;
		input_data.open(argv[2], ios::in);
		input_answer.open("testing_answer.txt",ios::in);
		output_accuracy.open("acc.txt",ios::out|ios::trunc);
		output_result.open(argv[3],ios::out|ios::trunc);



	/*
		Pre-process: 
			Calculate:
				frame_total: total frame # per sample , i.e. character counts per line in testing_data?.txt
				sample_total: total sample #, i.e. line counts in seq_model_0x.txt
				state_total: state number
				model_total: model number
	*/
	megumi = 0;
	while(input_data.get(buffer_char))
	{
		if(buffer_char == '\n')
		{
			frame_total = megumi;
			break;
		}
		megumi ++;
	}
	input_data.close();
	input_data.open(argv[2], ios::in);

	state_total = 6; //A~F, 6 in total
	model_total = 5; //01~05, 5 in total



	/*
		Variable declaration part 2: the mid product during testing the dataset
	*/
	double delta[frame_total][state_total];
	double probibility[model_total];
	double max_probibility = 0;

	int max_model;

	char data[frame_total];
	char answer[100];
	char buffer_str[100];
	char buffer_str1[100];

	HMM HMM_model[model_total];
	load_models( "modellist.txt", HMM_model, 5); //



	/*
		Main process, i.e. Loop all samples' data and answer
	*/
	while(!input_data.eof())
	{
		/*
			Load one sample's data and answer into data and answer, and check if eof
		*/
		input_data.getline(data, frame_total+1);
		input_answer.getline(answer, 100);
		if(input_data.eof())
		{
			break;
		}



		/*
			Loop all models
		*/		
		for(sharo=0; sharo<=model_total-1; sharo++)
		{
			/*
				Calculate: delta[frame_#][state_#]
			*/			
			for(cocoa=0; cocoa<=frame_total-1; cocoa++)
			{
				for(chiya=0; chiya<=state_total-1; chiya++)
				{
					delta[cocoa][chiya]=0;
				}
			}

			for(cocoa=0; cocoa<=frame_total-1; cocoa++)
			{
				state_number = (data[cocoa]) - 'A';
				if(cocoa == 0)
				{
					for(chiya=0; chiya<=state_total-1; chiya++)
					{
						delta[0][chiya] = HMM_model[sharo].initial[chiya] * HMM_model[sharo].observation[state_number][chiya];
					}
				}
				else
				{
					for(chiya=0; chiya<=state_total-1; chiya++)
					{
						for(chino=0; chino<=state_total-1; chino++)
						{
							if( (delta[cocoa-1][chino] * HMM_model[sharo].transition[chino][chiya] * HMM_model[sharo].observation[state_number][chiya]) > delta[cocoa][chiya] )
							{
								delta[cocoa][chiya] = delta[cocoa-1][chino] * HMM_model[sharo].transition[chino][chiya] * HMM_model[sharo].observation[state_number][chiya];
							}
						}
					}
				}
			}
			
			
			
			/*
				Calculate: probibility[model_#]
			*/			
			probibility[sharo] = 0;
			
			for(chiya=0; chiya<=state_total-1; chiya++)
			{
				if(delta[frame_total-1][chiya] > probibility[sharo])
				{
					probibility[sharo] = delta[frame_total-1][chiya];
				}
			}

		}

        
		/*
			Find the max_probibility and the max_model
		*/			
		max_model = 1; 
		max_probibility = probibility[0];
		for(sharo=1; sharo<=model_total-1; sharo++)
		{
			if(max_probibility < probibility[sharo])
			{
				max_probibility = probibility[sharo];
				max_model = sharo + 1;
			}
		}

		/*
			Output the result
		*/			
		output_result << "model_0" << max_model << ".txt\t" << max_probibility << "\n";
        
		/*
			Count the sample number and hits
		*/					
		sample_total ++;
		if(answer[7] == model2char[max_model-1])
		{
			correct_count ++;
		}
	}

	/*
		Output the accuracy, and then close akk files
	*/			
	output_accuracy << (correct_count/sample_total);
	input_data.close();
	input_answer.close();
	output_accuracy.close();
	output_result.close();

	return 0;
}
