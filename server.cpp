#include <arpa/inet.h>
#include <libexplain/read.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>  //to format the float value
#include <iostream>
#include <sstream>  //for appending operations
#include <string>
#define MAX 200
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
using namespace std;
int s_accout_number[MAX];
float s_amount[MAX];
string name[MAX];
char *c_ip;
int filectr;

void *calculate_interest(void *arg) {
     while (true) {
         sleep(60);

         cout << "\nAdding 10% Interest in every account after every 60 seconds\n";
         for (int i = 0; i < filectr; i++) {
            pthread_mutex_lock(&lock);
             s_amount[i] = s_amount[i] * 1.1;
             pthread_mutex_unlock(&lock);
         }
     }
 }

void *operations(void *arg) {
    stringstream amt_stream;
    stringstream amt_stream1;
    int *client_socket = (int *)arg;
    int accout_number;
    int timestamp;
    int amount;
    char transaction_type[1];

    char buff[4096];
    string msg_to_client;
    cout << "Client connected"
         << " " << *client_socket << " "
         << "Thread ID"
         << " " << pthread_self() << endl;
    int n;
    bzero(buff, 4096);
    int locked = 0;
    while (n = read(*client_socket, buff, 4096))
    {
        if (n < 0)
        {
            cout << "Error" << endl;
            break;
        }

        c_ip = strtok(buff, " ");
        if (c_ip)
        {
            timestamp = atoi(c_ip);
        }
        c_ip = strtok(NULL, " ");
        if (c_ip)
        {
            accout_number = atoi(c_ip);
        }
        c_ip = strtok(NULL, " ");
        if (c_ip)
        {
            strcpy(transaction_type, c_ip);
        }
        c_ip = strtok(NULL, " ");
        if (c_ip)
        {
            amount = stof(c_ip);
        }

        int current_account = -1;
        for (int i = 0; i < filectr; i++)
        {
            if (s_accout_number[i] == accout_number)
            {
                current_account = i;
                break;
            }
        }
        pthread_mutex_lock(&lock);
        if (current_account != -1)
        {
            
            if (strcmp(transaction_type, "d") == 0)
            {
                s_amount[current_account] = s_amount[current_account] + amount;
                string amt;
                string msg;
                amt.clear();
                msg.clear();
                std::stringstream stream;
                stream << std::fixed << std::setprecision(2) << s_amount[current_account];
                std::string s = stream.str();
                amt = s;
                msg = "Updated balence after deposite: \n";
                msg_to_client = msg+amt;
                write(*client_socket, msg_to_client.c_str(), msg_to_client.size() + 1);                               
            }
            else if (strcmp(transaction_type, "w") == 0)
            {
                if ((s_amount[current_account] - amount) < 0)
                {
                    msg_to_client = "Insufficient balence\n";
                    write(*client_socket, msg_to_client.c_str(), msg_to_client.size() + 1);
                }
                else
                {
                s_amount[current_account] = s_amount[current_account] - amount;
                string amt;
                string msg;
                amt.clear();
                msg.clear();
                std::stringstream stream;
                stream << std::fixed << std::setprecision(2) << s_amount[current_account];
                std::string s = stream.str();
                amt = s;
                msg = "Updated balence after withdrawal: \n";
                msg_to_client = msg+amt;
                write(*client_socket, msg_to_client.c_str(), msg_to_client.size() + 1); 
                }
            }
            else
            {
                msg_to_client = "Invalid transaction type\n";
                write(*client_socket, msg_to_client.c_str(), msg_to_client.size() + 1);
            }
            
        }

        else
        {
            msg_to_client = "Account number doesn't exist\n";
            write(*client_socket, msg_to_client.c_str(), msg_to_client.size() + 1);
        }
        pthread_mutex_unlock(&lock);
    }
}

int main() {  //For socket part I have referred below video
    //https://www.youtube.com/watch?v=cNdlrbZSkyQ
    pthread_t newthread[200];
    //Reading the file
    fstream accounts;
    accounts.open("./accounts");
    if (accounts.fail()) {
        cout << "Unable to read the file\n"
             << endl;
        exit(1);
    }
    string line;
    filectr = 0;
    while (getline(accounts, line)) {
        filectr++;
    }
    accounts.close();
    accounts.open("./accounts");
    for (size_t i = 0; i < filectr; i++) {
        accounts >> s_accout_number[i];
        accounts >> name[i];
        accounts >> s_amount[i];
    }
    accounts.close();

    //Create a server socket
    int listening = 0;
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1) {
        cerr << "socket not created\n";
    }

    else {
        cout << "Socket created with FD: " << listening << "\n";
    }

    int reuse_address = 1;
    //Below code is referred from: https://pubs.opengroup.org/onlinepubs/000095399/functions/setsockopt.html
    //To reuse the address
    /*if(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address)) != 0){
		cout<<"Failed to reuse the address"<<endl;
	}*/
    //To reuse the port
    if (setsockopt(listening, SOL_SOCKET, SO_REUSEPORT, &reuse_address, sizeof(reuse_address)) != 0) {
        cout << "Failed to reuse the port" << endl;
    }

    //Bind socket on ip & port
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54004);
    inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr);

    if (bind(listening, (sockaddr *)&hint, sizeof(hint)) == -1) {
        cerr << "Binding failed\n";
    }

    //Make the socket listen
    if (listen(listening, 4) == -1) {
        cerr << "Listening failed\n";
    }

    //accpet the connection
    sockaddr_in client;
    socklen_t clientsize = sizeof(client);
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    int clientsocket[200];
    for (int j =0; j < 200; j++) {
        clientsocket[j] = 0;
    }
    pthread_t interest;
    int i = 0;
    pthread_create(&interest, NULL, calculate_interest, NULL);
    while (true) {
        while (clientsocket[i] = accept(listening, (struct sockaddr *)&client, (socklen_t *)&clientsize)) {
            if (clientsocket[i] == -1) {
                cerr << "Unable to connect with client\n";
                continue;
            } else {
                pthread_create(&newthread[i], NULL, operations, &clientsocket[i]);
                i++;
            }
        }
        cout << "closing " << clientsocket << endl;
        close(clientsocket[i]);
        close(listening);
    }

    return 0;
}
