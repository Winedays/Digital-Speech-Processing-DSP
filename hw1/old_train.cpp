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
	int state_num;
	int iteration_total = 0;
	int sample_total = 0;
	int frame_total = 0;
	int state_total = 0;
	int sharo, cocoa, chino, chiya, rize, maya, megumi;
		// chino for iteration index;
		// sharo for sample index;
		// cocoa, maya for frame index;
		// chiya, rize for state index;
		// megumi for other;

	char **data, buffer_char;
	char buffer_str[100];
	char buffer_str1[100];

	fstream input_data;
	FILE *output_model = fopen("model_01.txt","w");
	HMM HMM_model; loadHMM(&HMM_model, "model_init.txt");



	/*
		Pre-process:
			Calculate:
				iteration_total: iteration number
				frame_total: total frame # per sample , i.e. character counts per line in seq_model_0x.txt
				sample_total: total sample #, i.e. line counts in seq_model_0x.txt
				state_total: state number
			And load all the data in seq_model_0x.txt into data[sample_#][frame_#] )
	*/
	iteration_total = 100;

	megumi = 0;
	input_data.open("seq_model_01.txt", ios::in);//argv[3] == seq_model_*
	while(input_data.get(buffer_char))
	{
		if(buffer_char == '\n')
		{
			frame_total = megumi;
			break;
		}
		megumi ++;
	}//看起來單純是計算目前這seq_model_0* 每一行(observation)共多長(幾個char)
	//megumi=50
	/*
	可改成
	string line;
	getline(input_data,line);
	megumi = line.size();
	*///megumi單純一個temp 變數
	input_data.close();

	input_data.open("seq_model_01.txt", ios::in);
	while(!input_data.eof())
	{
		input_data.getline(buffer_str, sizeof(buffer_str));
		//每一次的buffer_str就是txt中的每一行DATA(50個字母)
		sample_total++;
	}
	sample_total--;//此seq_model_*共有幾組obeservation == 10000組 ,sample_total=10000
	input_data.close();


	state_total = 6; //A~F, 6 totally

	data = new char *[sample_total];
	for(sharo=0; sharo<sample_total; sharo++)
	{
		data[sharo] = new char[frame_total+1];
	 	//data[0~9999] = new char[50+1]  data[][]為一個2d char , 這邊在做Initializing
	}


	// sharo for sample index;
	// cocoa, maya for frame index;
	sharo = 0;
	cocoa = 0;
	input_data.open("seq_model_01.txt",ios::in );
	while(input_data.get(buffer_char))
	{
		if(buffer_char == '\n')
		{
			data[sharo][cocoa] = 'n';
			frame_total = cocoa;
			sharo++;
			cocoa = 0;
			continue;
		}
		data[sharo][cocoa] = buffer_char;// sharo=0~9999 , cocoa=0~49(大約)
		cocoa++;
	}
	input_data.close();



	/*
		Variable declaration part 2: the mid product during calculate the model parameters
	*/
	double alpha[50+1][6+1];//[50+1][6+1]
	double beta[50+1][6+1];
	double gama[50+1][6+1];
	double epsilon[50][6+1][6+1];//[50+1][6+1][6+1]

	double sum_alpha_1;
	double sum_gama_1[6+1];//[6+1]
	double sum_gama_2[6+1];
	double sum_gama_3[6+1];
	double sum_gama_all_state[6][6+1];//[6+1][6+1]
	double sum_epsilon_1[6+1][6+1];



	/*
		The main process, i.e. loop all iterations
	*/
	for(chino=1; chino<=iteration_total; chino++)//iteration幾次
	{
		cout << "The " << chino << "th iteration is now processing..." <<  endl;

		/*
			Initializing some variables
		*/
		for(chiya=1; chiya<=state_total; chiya++)
		{
			sum_gama_1[chiya] = 0;
			sum_gama_2[chiya] = 0;
			sum_gama_3[chiya] = 0;
			for(rize=1; rize<=state_total; rize++)
			{
				sum_epsilon_1[chiya][rize] = 0;
				sum_gama_all_state[rize-1][chiya] = 0;
			}
		}
		sum_alpha_1 = 0;


		/*
			Loop all samples
		*/
		for(sharo=0; sharo<sample_total; sharo++)
		{
			/*
				Calculate:
					alpha[frame_#][state_#]  .. [50][6] ///frame的意思有點類似ppt forward backward中的x軸1~T
					beta[frame_#][state_#]   .. [50][6]
			*/
			for(cocoa=0; cocoa<=frame_total-1; cocoa++) //0~50-1
			{
				if(cocoa == 0)
				{
					state_num = (data[sharo][0]) - 'A';//initial state = data[0][0]
					for(chiya=1; chiya<=state_total; chiya++)
					{
						//alpha Initializing , 初始state機率*目前此初始state看到此obesrvation的機率
						alpha[1][chiya] = HMM_model.initial[chiya-1] * HMM_model.observation[state_num][chiya-1];
						//beta Initializing
						beta[frame_total][chiya] = 1;//initial 最後一行（backward algorithm）
					}
				}
				else
				{
					for(maya=1; maya<=frame_total-1; maya++) //1~50-1
					{
						for(chiya=1; chiya<=state_total; chiya++)//1~6
						{
							state_num = (data[sharo][maya]) - 'A';//ex: 'C'-'A' == 2
							alpha[maya+1][chiya] = 0;//forward的下一行
							for(rize=1; rize<=state_total; rize++)
							{
								alpha[maya+1][chiya] += alpha[maya][rize] * HMM_model.transition[rize-1][chiya-1];
							}
							alpha[maya+1][chiya] *= HMM_model.observation[state_num][chiya-1];

							state_num = (data[sharo][frame_total-maya]) - 'A';
							beta[frame_total-maya][chiya] = 0;
							for(rize=1; rize<=state_total; rize++)
							{
								beta[frame_total-maya][chiya] +=  HMM_model.transition[chiya-1][rize-1] * HMM_model.observation[state_num][rize-1] * beta[frame_total+1-maya][rize];
							}
						}
					}
				}
			}
			// alpha & beta end 
			
			


			/*
				Calculate: gamma[frame_#][state_#]
			*/
			sum_alpha_1 = 0;
			for(chiya=1; chiya<=state_total; chiya++)
			{
				sum_alpha_1 += alpha[frame_total][chiya];
			}
			for(cocoa=1; cocoa<=frame_total; cocoa++)
			{
				for(chiya=1; chiya<=state_total; chiya++)
				{
					gama[cocoa][chiya] = alpha[cocoa][chiya] * beta[cocoa][chiya] / sum_alpha_1;
				}
			}


			/*
				Calculate: epsilon[frame_#][state_#][state_#]
			*/
			for(cocoa=1; cocoa<=frame_total-1; cocoa++)
			{
				state_num = (data[sharo][cocoa]) - 'A';
				for(chiya=1; chiya<=state_total; chiya++)
				{
					for(rize=1; rize<=state_total; rize++)
					{
						epsilon[cocoa][chiya][rize] = alpha[cocoa][chiya] * HMM_model.transition[chiya-1][rize-1] * HMM_model.observation[state_num][rize-1] * beta[cocoa+1][rize] / sum_alpha_1;
						if (cocoa == 1 && chiya == 1 && rize == 3) {
							cout << alpha[cocoa][chiya] << "+" << HMM_model.transition[chiya - 1][rize - 1] << "+" << HMM_model.observation[state_num][rize - 1] << "+" << beta[cocoa + 1][rize] << "+" << sum_alpha_1 << "+";
						}
					}
				}
			}
			if (chino == 1 && sharo == 0) {
				cout << "epsilon : " << endl;
				for (cocoa = 0; cocoa <= frame_total; cocoa++)
				{
					// run all state to cal. alpha & beta
					for (chiya = 1; chiya <= state_total; chiya++)
					{
						for (rize = 1; rize <= state_total; rize++)
						{
							//cal. alpha_t+1(j) , beta_t-1(j)
							cout << epsilon[cocoa][chiya][rize] << " ";
						}
						
					}
					cout << endl;
				}
				cout << endl;
				cout << "epsilon end " << endl;
			}

			/*
				Calculate:
					sum_gama_1[state_#]: sum_all_sample( gama[1][state_#] )
					sum_gama_2[state_#]: sum_all_sample( sum( gama[1:frame_total-1][state_#] ) )
					sum_gama_{A~F}[state_#]: sum_all_sample( sum( gama[frame_#][state_#] ) ), where data[sample_#][frame_#-1] is that state
					sum_epsilon_1[state_#][state_#]: sum_all_sample( sum( epsilon[:][state_#][state_#] ) )
			*/
			for(chiya=1; chiya<=state_total; chiya++)
			{
				sum_gama_1[chiya] += gama[1][chiya];
				for(cocoa=1; cocoa<=frame_total; cocoa++)
				{
					sum_gama_3[chiya] += gama[cocoa][chiya];

					sum_gama_all_state[ (data[sharo][cocoa-1]-'A') ][chiya] += gama[cocoa][chiya];
					if(cocoa != frame_total)
					{
						sum_gama_2[chiya] += gama[cocoa][chiya];
					}
				}
				for(rize=1; rize<=state_total; rize++)
				{
					for(cocoa=1; cocoa<=frame_total-1; cocoa++)
					{
						sum_epsilon_1[chiya][rize] += epsilon[cocoa][chiya][rize];
					}
				}
			}
		}


		/*
			Calculate: the 3 parameter of the model:
				HMM_model.transition[state_#][state_#]: parameter A in the HMM model
				HMM_model.observation[state_#][state_#]: parameter B in the HMM model
				HMM_model.initial[state_#]: parameter pi in the HMM model
		*/
		for(chiya=0; chiya<=state_total-1; chiya++)
		{
			HMM_model.initial[chiya] = sum_gama_1[chiya+1] / static_cast<double>(sharo);

			for(rize=0; rize<=state_total-1; rize++)
			{
				HMM_model.transition[chiya][rize] = sum_epsilon_1[chiya+1][rize+1] / sum_gama_2[chiya+1];
				HMM_model.observation[rize][chiya] = sum_gama_all_state[rize][chiya+1] / sum_gama_3[chiya+1];
			}
		}
		dumpHMM(stderr, &HMM_model);

	}




	/*
		Post-process: Save the model
	*/
	dumpHMM(output_model, &HMM_model);
	fclose(output_model);

	return 0;
}
