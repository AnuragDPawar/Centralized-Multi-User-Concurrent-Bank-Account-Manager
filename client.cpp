#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <stdio.h>
#include <fstream>
#include <string.h>

using namespace std;
void writeTimestamp(float &, int);
void writeTimestamp(float &operation_time, int totalSize){
	//commented as the below code is used for testing scalability
	ofstream file1;
	file1.open("file.txt",ios::app);
	if(!file1.is_open()){
		cout << endl << "log file could not be opened" <<endl;
		exit(0);
	}
    else
    {
        cout<<"File is opened for time\n";
    }
	file1  << " " + to_string(operation_time/totalSize) << "\n";
	file1.close();
}



int main()
{   
    string first_item;
    char object[99];
    int no_of_lines =0;
    int start_timer, stop_timer;
    float store_time[200];
    //reading the transaction file
    fstream transactions;
    transactions.open("./transactions");
    if (transactions.fail())
    {
        cout << "Unable to read the file\n" << endl;
        exit(1);
    }    
    transactions.close();
    //creatomg socket
    int csock = socket(AF_INET, SOCK_STREAM,0);
    if (csock == -1)
    {
        cerr << "socket not created\n";
    }

    int port = 54004;
    string IP = "127.0.0.1";
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, IP.c_str(), &hint.sin_addr);

    //connecting to the server
    int connreq = connect(csock,(sockaddr *)&hint, sizeof(sockaddr_in));

    transactions.open("./transactions");
    while (getline (transactions,first_item))
    {
        no_of_lines++;
    }
    cout<<no_of_lines<<endl;
    transactions.close();
    //while loop
    char buf[4096];
    transactions.open("./transactions");
    for (int i=0;i<no_of_lines;i++)
    {
        //send to server
        getline (transactions,first_item);
        start_timer=clock(); //start the counting of the time
        int send_resquest_to_server = write(csock, first_item.c_str(), first_item.size()+1);
        cout<<endl;
        cout<<first_item<<endl;
        if(send_resquest_to_server==-1)
        {
            cout<<"Error in sending\n";
        }
        
        //wait for response
        memset(buf,0,4096);
        int recvmsg = recv(csock, buf, 4096,0);
        //display response
        cout<<"Message from server: "<<string(buf,recvmsg)<<"\t\n";
        stop_timer=clock(); //stop the counting of the time
        store_time[i]= ((stop_timer-start_timer)/double(CLOCKS_PER_SEC)*1000);//array stores the time taken by each operation
        sleep(2);
    }

    float operation_time= 0;
	for(int i=0;i<no_of_lines;i++){
		operation_time+=store_time[i];// used for calculating average time taken per operation
	}
    cout<<"\nTime in mili seconds: "<<operation_time<<endl;
    writeTimestamp(operation_time, no_of_lines);

    close(csock);
    
    return 0;
}

//Time code referrence: https://github.com/deepikarajani24/Centralized-Multi-User-Concurrent-Banking-System/blob/master/src/client.cpp