#include "hmm.h"
#include <math.h>
#include <float.h>
#include <iostream>
using namespace std;

int read_data(char *file, char *data[]);
double** malloc2Dparameter(int row_size, int col_size);
void fillzero1D(double* data, int row_size);
void fillzero2D(double** data, int row_size, int col_size);

int main(int argc, char *argv[])
{
	char* model_list;
	char* seq_data;
	char* out_result;

	// init. file path
	if (argc == 1) {
		// if no argument variable , init. file path
		char* model_name = "modellist.txt";
		model_list = new char[strlen(model_name)];
		strcpy(model_list, model_name);

		model_name = "testing_data1.txt";
		seq_data = new char[strlen(model_name)];
		strcpy(seq_data, model_name);

		model_name = "result1.txt";
		out_result = new char[strlen(model_name)];
		strcpy(out_result, model_name);
	}
	else {
		model_list = argv[1];
		seq_data = argv[2];
		out_result = argv[3];
	}


	HMM hmms[5];
	load_models(model_list, hmms, 5);
	//dump_models(hmms, 5);

	/*
	HMM hmm_initial;
	loadHMM( &hmm_initial, "model_init.txt" );
	dumpHMM( stderr, &hmm_initial );
	*/

	char *test_data[20000];
	int data_count = read_data(seq_data, test_data);

	// init. parameter
	int state_size = hmms[0].state_num;

	double** delta = malloc2Dparameter(strlen(test_data[0]), state_size) ;
	double* model_probibility = (double *)malloc(sizeof(double) * 5);
	double** result_list = malloc2Dparameter(data_count, 2);  // save the resukt for each sample r[0] is answer model , r[1] is its porb.


	// run all sample
	for (size_t sample = 0; sample < data_count; sample++)
	{
		char *data = test_data[sample];
		fillzero1D(model_probibility, 5) ;

		// run all models
		for (size_t model = 0; model < 5; model++)
		{
			HMM hmm = hmms[model] ;
			fillzero2D(delta, strlen(test_data[0]), state_size) ;

			// delta
			//init. delta_1
			int data_state = data[0] - 'A';
			for (size_t state = 0; state < state_size; state++)
			{
				delta[0][state] = hmm.initial[state] * hmm.observation[data_state][state] ;
			}
			//run all timepoint in data
			for (size_t t = 1; t < strlen(data); t++)
			{
				data_state = data[t] - 'A';
				// run all state to cal. delta
				for (size_t s_j = 0; s_j < state_size; s_j++)
				{
					double temp_delta = -1.0 ;
					// find the max delta_t-1[i] * s[i][j]
					for (size_t s_i = 0; s_i < state_size; s_i++)
					{
						if (temp_delta < delta[t - 1][s_i] * hmm.transition[s_i][s_j])
						{
							temp_delta = delta[t - 1][s_i] * hmm.transition[s_i][s_j] ;
						}
					}
					delta[t][s_j] = temp_delta * hmm.observation[data_state][s_j] ;
				}
			}
			// delta end

			// cal. each model prob.
			double temp_porb = 0. ;
			for (size_t state = 0; state < state_size; state++)
			{
				if (temp_porb < delta[strlen(data)-1][state])
				{
					temp_porb = delta[strlen(data) - 1][state] ;
				}
			}
			model_probibility[model] = temp_porb ;
		}
		// run all models end

		// find the max prob. model and save it result
		int model_num = 0 ;
		double temp_prob = 0 ;
		for (size_t model = 0; model < 5; model++)
		{
			if (temp_prob < model_probibility[model]) 
			{
				temp_prob = model_probibility[model] ;
				model_num = model+1 ;
			}
		}
		result_list[sample][0] = model_num ;
		result_list[sample][1] = model_probibility[model_num] ;

	}
	// run all sample end

	// save result to file
	FILE *fp = fopen(out_result , "w");     /* open file pointer */
	for (size_t sample = 0; sample < data_count; sample++)
	{
		fprintf(fp, "model_0%d.txt %.17g\n", (int)result_list[sample][0], result_list[sample][1]) ;
	}
	fclose(fp) ;

	// if it is test data set 1 , cal. the accuracy
	if (strcmp(seq_data, "testing_data1.txt") == 0)
	{
		double acc_count = 0;
		fp = open_or_die("testing_answer.txt", "r");
		char token[13];
		int sample_count = 0 ;
		while (fscanf(fp, "%s", token) > 0)
		{
			if (token[0] == '\0' || token[0] == '\n') continue;
			int corr_ans = token[7] - '0' ;
			if (corr_ans  == (int)(result_list[sample_count][0]) )
			{
				acc_count++ ;
			}
			sample_count++ ;
		}
		fclose(fp) ;
		acc_count /= (1.0 * data_count) ;

		// save accuracy
		fp = fopen("acc.txt", "w");     /* open file pointer */
		fprintf(fp, "%lf\n", acc_count);
		fclose(fp);
		cout << "acc_count : " << acc_count << endl ;
	}
	// cal. the accuracy end

	// system("pause");
}

// read seq file 
int read_data(char *file, char *data[])
{
	FILE *fp = open_or_die(file, "r");

	char token[100];
	for (int i = 0; i < 20000; i++) {
		data[i] = (char *)malloc(sizeof(char) * 100);
	}

	int seq_count = 0;
	while (fscanf(fp, "%s", token) > 0)
	{
		if (token[0] == '\0' || token[0] == '\n') continue;

		strcpy(data[seq_count], token);
		seq_count++;
	}
	fclose(fp);
	return seq_count;
}

// malloc a 2D parameter
double** malloc2Dparameter(int row_size, int col_size) {
	double **data = (double **)malloc(sizeof(double*) * row_size);
	for (size_t t = 0; t < row_size; t++)
	{
		data[t] = (double *)malloc(sizeof(double) * col_size);
	}
	return data;
}

// 1D array fill zero
void fillzero1D(double* data, int row_size) {
	for (size_t i = 0; i < row_size; i++)
	{
		data[i] = 0;
	}
	//return data;
}

// 2D array fill zero
void fillzero2D(double** data, int row_size, int col_size) {
	for (size_t i = 0; i < row_size; i++)
	{
		for (size_t j = 0; j < col_size; j++)
		{
			data[i][j] = 0;
		}
	}
	//return data;
}

