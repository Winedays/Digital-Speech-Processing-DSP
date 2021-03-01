#include "hmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
using namespace std;

int read_data(char *file, char *data[]);
double** malloc2Dparameter(int row_size, int col_size);
double*** malloc3Dparameter(int size_3d, int size_2d, int size_1d);
void fillzero1D(double* data, int row_size);
void fillzero2D(double** data, int row_size, int col_size);
void fillzero3D(double*** data, int size_3d, int size_2d, int size_1d);

//argc琌argument count(把计计)罽糶珹セō把计计╰参穦笆璸衡┮块把计计
int main(int argc, char *argv[])
{
	int iteration;
	char* init_model;
	char* seq_model;
	char* out_model;

	// init. file path
	if(argc == 1){
		// if no argument variable , init. file path
		iteration = 100 ;

		char* model_name = "model_init.txt";
		init_model = new char[strlen(model_name)];
		strcpy(init_model, model_name);

		model_name = "seq_model_01.txt";
		seq_model = new char[strlen(model_name)];
		strcpy(seq_model, model_name);

		model_name = "model_01.txt";
		out_model = new char[strlen(model_name)];
		strcpy(out_model, model_name);
	}
	else {
		iteration = atoi( argv[1] ) ;
		init_model = argv[2] ;
		seq_model = argv[3] ;
		out_model = argv[4] ;
	}
	
	// read model file
	HMM hmm_initial;
	loadHMM(&hmm_initial, init_model);
	dumpHMM(stderr, &hmm_initial);

	char *train_data[20000] ;
	int data_count = read_data(seq_model, train_data) ;

	/*cout << iteration << endl;
	cout << init_model << endl;
	cout << seq_model << endl ;
	cout << out_model << endl ;*/
	
	/*for (int i = 0; i < data_count; i++) {
		cout << train_data[i] << endl;
	}*/

	/*for (int i = 0; i < hmm_initial.state_num; i++) {
		cout << hmm_initial.initial[i] << "  ";
	}
	cout << endl << endl;
	for (int i = 0; i < hmm_initial.observ_num; i++) {
		for (int j = 0; j < hmm_initial.observ_num; j++) {
			cout << hmm_initial.transition[i][j] << "  ";
		}
		cout << endl;
	}
	cout << endl;
	for (int i = 0; i < hmm_initial.observ_num; i++) {
		for (int j = 0; j < hmm_initial.observ_num; j++) {
			cout << hmm_initial.observation[i][j] << "  ";
		}
		cout << endl;
	}
	cout << endl;*/

	// init. parameter
	int state_size = hmm_initial.state_num;

	double **alpha = malloc2Dparameter(strlen(train_data[0]), state_size);
	double **beta = malloc2Dparameter(strlen(train_data[0]), state_size);
	double **gamma = malloc2Dparameter(strlen(train_data[0]), state_size);
	double ***epsilon = malloc3Dparameter(strlen(train_data[0]), state_size, state_size);

	double** sample_sum_gamma = malloc2Dparameter(strlen(train_data[0]), state_size);
	double*** sample_sum_epsilon = malloc3Dparameter(strlen(train_data[0]), state_size, state_size);
	double** sample_sum_gamma_state = malloc2Dparameter(state_size, state_size);
	// traing with iteration times
	for (size_t i = 0; i < iteration; i++)
	{
		cout << "iteration : " << i << endl;

		fillzero2D(sample_sum_gamma, strlen(train_data[0]),state_size) ;
		fillzero3D(sample_sum_epsilon, strlen(train_data[0]), state_size, state_size) ;
		fillzero2D(sample_sum_gamma_state, state_size, state_size);

		// run all sample
		for (size_t j = 0; j < data_count; j++)
		{
			//cout << "sample : " << j << endl;

			char *data = train_data[j] ;
			
			// init parameter
			int data_state = data[0] - 'A';
			fillzero2D(alpha, strlen(train_data[0]), state_size);
			fillzero2D(beta, strlen(train_data[0]), state_size);
			fillzero2D(gamma, strlen(train_data[0]), state_size);
			fillzero3D(epsilon, strlen(train_data[0]), state_size, state_size);


			// init alpha[0] & beta[T]
			for (size_t state = 0; state < state_size; state++)
			{
				alpha[0][state] = hmm_initial.initial[state] * hmm_initial.observation[data_state][state] ;
				beta[strlen(data)-1][state] = 1.0 ;
			}

			// alpha & beta
			// run all timepoint in data (t) to cal. alpha & beta
			for (size_t t = 1; t < strlen(data); t++)
			{
				data_state = data[t] - 'A' ;
				int data_state_beta = data[strlen(data) - t] - 'A';
				// run all state to cal. alpha & beta
				for (size_t state = 0; state < state_size; state++)
				{
					//cal. alpha_t+1(j) , beta_t-1(j)
					double temp_alpha = 0 , temp_beta = 0 ;
					for (size_t s = 0; s < state_size; s++)
					{
						temp_alpha += alpha[t-1][s] * hmm_initial.transition[s][state] ;
						temp_beta += hmm_initial.transition[state][s] * hmm_initial.observation[data_state_beta][s] * beta[strlen(data) - t][s];
					}
					alpha[t][state] = temp_alpha * hmm_initial.observation[data_state][state];
					beta[strlen(data)-1-t][state] = temp_beta;
				}
			}
			// alpha & beta end

			// gamma & epsilon
			// run all timepoint in data (t) to cal. gamma & epsilon
			for (size_t t = 0; t < strlen(data); t++)
			{
				//cal. gamma
				// we got alpha & beta , run all state to cal. gamma
				double temp_gamma = 0;
				for (size_t state = 0; state < state_size; state++)
				{
					//temp_gamma += alpha[t][state] * beta[t][state];  // ??? 
					temp_gamma += alpha[strlen(data)-1][state];  // ??? 
				}
				//cal. gamma_t
				for (size_t state = 0; state < state_size; state++)
				{
					gamma[t][state] = (alpha[t][state] * beta[t][state]) / temp_gamma;   // ??? 
				}

				//cal. epsilon
				// we got alpha & beta , run all state to cal. epsilon
				double temp_epsilon = temp_gamma ; // we know the temp_gamma is equal to temp_epsilon
				//cal. epsilon_t
				data_state = data[t+1] - 'A';
				if ( t != strlen(data)-1 )
				{
					for (size_t s_i = 0; s_i < state_size; s_i++)
					{
						for (size_t s_j = 0; s_j < state_size; s_j++)
						{
							epsilon[t][s_i][s_j] = (alpha[t][s_i] * hmm_initial.transition[s_i][s_j] * hmm_initial.observation[data_state][s_j] * beta[t+1][s_j]) / temp_epsilon;
							
						}
					}
				}
			}
			// gamma & epsilon end
			
			// sum all sample data of gamma & epsilon
			for (size_t t = 0; t < strlen(data); t++)
			{
				data_state = data[t] - 'A'; 

				for (size_t s_i = 0; s_i < state_size; s_i++)
				{
					sample_sum_gamma[t][s_i] += gamma[t][s_i] ;
					sample_sum_gamma_state[data_state][s_i] += gamma[t][s_i] ;
					if (t != strlen(data) - 1)
					{
						for (size_t s_j = 0; s_j < state_size; s_j++)
						{
							sample_sum_epsilon[t][s_i][s_j] += epsilon[t][s_i][s_j];
						}
					}
				}
			}

		}
		// run all sample end

		// cal. the new model
		double* pi_new = (double *)malloc(sizeof(double) * hmm_initial.state_num);
		double** a_new = malloc2Dparameter(hmm_initial.observ_num, hmm_initial.observ_num);
		double** b_new = malloc2Dparameter(hmm_initial.observ_num, hmm_initial.observ_num);
		fillzero1D(pi_new, state_size);
		// cal. pi
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			pi_new[s_i] = sample_sum_gamma[0][s_i] / (double)(1.0 * data_count);
		}
		// cal. a
		double* temp_gamma = (double *)malloc(sizeof(double) * state_size);
		double** temp_epsilon = malloc2Dparameter(state_size, state_size);
		fillzero1D(temp_gamma, state_size);
		fillzero2D(temp_epsilon, state_size, state_size);
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			for (size_t t = 0; t < strlen(train_data[0]) - 1; t++)
			{
				temp_gamma[s_i] += sample_sum_gamma[t][s_i];
			}
			for (size_t s_j = 0; s_j < state_size; s_j++)
			{
				for (size_t t = 0; t < strlen(train_data[0]) - 1; t++)
				{
					temp_epsilon[s_i][s_j] += sample_sum_epsilon[t][s_i][s_j];
				}
			}
		}
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			for (size_t s_j = 0; s_j < state_size; s_j++)
			{
				a_new[s_i][s_j] = temp_epsilon[s_i][s_j] / temp_gamma[s_i];
			}
		}
		// cal. b
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			temp_gamma[s_i] += sample_sum_gamma[strlen(train_data[0]) - 1][s_i];
		}
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			for (size_t s_j = 0; s_j < state_size; s_j++)
			{
				b_new[s_j][s_i] = sample_sum_gamma_state[s_j][s_i] / temp_gamma[s_i];
			}
		}
		// cal. the new model end

		// update model
		for (size_t s_i = 0; s_i < state_size; s_i++)
		{
			hmm_initial.initial[s_i] = pi_new[s_i];
			for (size_t s_j = 0; s_j < state_size; s_j++)
			{
				hmm_initial.transition[s_i][s_j] = a_new[s_i][s_j];
				hmm_initial.observation[s_j][s_i] = b_new[s_j][s_i];
			}
		}
		dumpHMM(stderr, &hmm_initial);

	}
	// traing with iteration times end

	// save model
	FILE *train_model = fopen(out_model, "w");
	dumpHMM(train_model, &hmm_initial);
	fclose(train_model);

	//system("pause");
}

// read seq file 
int read_data(char *file , char *data[])
{
	FILE *fp = open_or_die(file, "r");

	char token[100] ;
	for (int i = 0; i < 20000; i++) {
		data[i] = (char *)malloc(sizeof(char) * 100 ) ;
	}

	int seq_count = 0 ;
	while (fscanf(fp, "%s", token) > 0)
	{
		if (token[0] == '\0' || token[0] == '\n') continue;

		strcpy(data[seq_count], token);
		seq_count++ ;
	}
	fclose(fp);
	return seq_count ;
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

// malloc a 3D parameter
double*** malloc3Dparameter(int size_3d, int size_2d, int size_1d) {
	double ***data = (double ***)malloc(sizeof(double**) * size_3d);
	for (size_t i = 0; i < size_3d; i++)
	{
		data[i] = malloc2Dparameter(size_2d, size_1d);
		/*data[i] = (double **)malloc(sizeof(double*) * size_2d);
		for (size_t j = 0; j < size_1d; j++)
		{
			data[i][j] = (double *)malloc(sizeof(double) * size_1d);
		}*/
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

// 3D array fill zero
void fillzero3D(double*** data, int size_3d, int size_2d, int size_1d) {
	for (size_t i = 0; i < size_3d; i++)
	{
		fillzero2D(data[i], size_2d, size_1d) ;
		/*for (size_t j = 0; j < size_2d; j++)
		{
			for (size_t k = 0; k < size_1d; k++)
			{
				data[i][j][k] = 0;
			}
		}*/
	}
	//return data;
}

// release
