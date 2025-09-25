/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Nathaniel Shipman
	UIN:333009078
	Date:9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool input_p = false;
	bool input_t = false;
	bool input_e = false;
	bool input_f = false;
	bool input_c = false;

	pid_t pid = fork();
	char *args[] = {(char*)"./server", nullptr};
	if (pid == 0){
		execvp("./server", args);
	}
	
	string filename = "";
	while ((opt = getopt(argc, argv, ":cp:t:e:f:")) != -1) {
		switch (opt) {
			case 'c':
				input_c = true;
				break;
			case 'p':
				p = atoi (optarg);
				input_p = true;
				break;
			case 't':
				t = atof (optarg);
				input_t = true;
				break;
			case 'e':
				e = atoi (optarg);
				input_e = true;
				break;
			case 'f':
				filename = optarg;
				input_f = true;
				break;
		}
	}

	FIFORequestChannel** channels = new FIFORequestChannel*[2]();
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	channels[0] = chan;
	if (input_c){
		MESSAGE_TYPE channel_message = NEWCHANNEL_MSG;
		char new_channel[30];
		chan->cwrite(&channel_message, sizeof(MESSAGE_TYPE));
		chan->cread(new_channel, 30);
		string new_channel_string(new_channel);
		FIFORequestChannel* new_chan = new FIFORequestChannel(new_channel_string, FIFORequestChannel::CLIENT_SIDE);
		channels[1] = new_chan;
		chan = new_chan;
	}

	if (input_p || input_t || input_e || (!input_p && !input_t && !input_e && !input_f)){
		// example data point request
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);
		double reply;
		ofstream csv_file;
		
		if(!input_t && !input_e && input_p){
			csv_file.open("received/x1.csv");
			if (csv_file.is_open()){
				for (t = 0; t <= 4.0; t += 0.004){
					x.seconds = t;
					csv_file << t;
					for (e = 1; e <= 2; e++){
						x.ecgno = e;
						memcpy(buf, &x, sizeof(datamsg));
						chan->cwrite(buf, sizeof(datamsg)); // question
						chan->cread(&reply, sizeof(double)); // answer
						csv_file << "," << reply;
					}
					csv_file << '\n';
				}
			}
			csv_file.close();
		}
		else{
			memcpy(buf, &x, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			chan->cread(&reply, sizeof(double)); //answer
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
	}

	if (input_f){	
		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan->cwrite(buf2, len);  // I want the file length;
		__int64_t file_size;
		chan->cread(&file_size, sizeof(__int64_t));

		__int64_t num_offsets = file_size / (MAX_MESSAGE);
		int message_remainder = file_size % (MAX_MESSAGE);
		char buf3[MAX_MESSAGE+1];
		FILE* new_file;
		string received_file = "received/" + filename;
		new_file = fopen(received_file.data(), "w+");
		for (int i = 0; i < num_offsets; i++){
			fm.offset = i * (MAX_MESSAGE);
			fm.length = MAX_MESSAGE;
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan->cwrite(buf2, len);
			chan->cread(buf3, MAX_MESSAGE);
			fwrite(buf3, sizeof(char), MAX_MESSAGE, new_file);
		}
		
		char buf_remain[MAX_MESSAGE];
		if (message_remainder > 0){
			fm.offset = num_offsets * (MAX_MESSAGE);
			fm.length = message_remainder;
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), fname.c_str());
			chan->cwrite(buf2, len);
			chan->cread(buf_remain, MAX_MESSAGE);
			fwrite(buf_remain, sizeof(char), message_remainder, new_file);
		}

		pid_t pid2 = fork();
		string file_original = "BIMDC/" + filename;
		string file_copy = "received/" + filename;
		char* args2[] = {(char*)"diff", file_original.data(), file_copy.data(), nullptr};
		if (pid2 == 0){
			execvp("diff", args2);
		}

		fclose(new_file);
		delete[] buf2;
	}
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan->cwrite(&m, sizeof(MESSAGE_TYPE));
    for (int i = 0; i < 2; i++){
		if (channels[i] != nullptr){
			delete channels[i];
		}
	}
	delete[] channels;

}
