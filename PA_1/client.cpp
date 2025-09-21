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

	pid_t pid = fork();
	char *args[] = {(char*)"./server", nullptr};
	if (pid == 0){
		execvp("./server", args);
	}
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
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

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
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
					chan.cwrite(buf, sizeof(datamsg)); // question
					chan.cread(&reply, sizeof(double)); // answer
					csv_file << "," << reply;
				}
				csv_file << '\n';
			}
		}
		csv_file.close();
	}
	else{
    		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
if (input_f){	
    // sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = filename;
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;
	__int64_t file_size;
	chan.cread(&file_size, sizeof(__int64_t));

	__int64_t num_offsets = file_size / (MAX_MESSAGE);
	int message_remainder = file_size % (MAX_MESSAGE);
	char buf3[MAX_MESSAGE+1];
	FILE* new_file; //ofstream new_file;
	string received_file = "received/" + filename;
	new_file = fopen(received_file.data(), "w+"); //new_file.open("received/" + filename);
	for (int i = 0; i < num_offsets; i++){
		fm.offset = i * (MAX_MESSAGE);
		fm.length = MAX_MESSAGE;
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);
		chan.cread(buf3, MAX_MESSAGE);
		fwrite(buf3, sizeof(char), MAX_MESSAGE, new_file); //new_file << buf3;
	}
	
	char buf_remain[MAX_MESSAGE];
	if (message_remainder > 0){
		fm.offset = num_offsets * (MAX_MESSAGE);
		fm.length = message_remainder;
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);
		chan.cread(buf_remain, /*message_remainder*/MAX_MESSAGE);
		fwrite(buf_remain, sizeof(char), message_remainder, new_file); //new_file << buf_remain;
	}

	pid_t pid2 = fork();
	string file_original = "BIMDC/" + filename;
	string file_copy = "received/" + filename;
	char* args2[] = {(char*)"diff", file_original.data(), file_copy.data(), nullptr};
	if (pid2 == 0){
		execvp("diff", args2);
	}

	/*pid_t pid3 = fork();
	char* args3[] = {(char*)"truncate", (char*)"-s", to_string(file_size).data(), (char*)"BIMDC/test.bin" , nullptr};
	if (pid3 == 0){
		execvp("truncate", args3);
	}*/
	fclose(new_file); //new_file.close();
	delete[] buf2;
}
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
    //wait(NULL);
}
