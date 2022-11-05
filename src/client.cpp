// CLIENT

#include <iostream>
#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "helpers.h"

#define STDBY_TIME 3000

using namespace std;

int socket_fd;
struct sockaddr_in server_addr, client_addr;

int window_len = 5;

socklen_t server_addr_size = sizeof(server_addr);

void send_ack()
{
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frame_size;
    int data_size;

    int recv_seq_num;
    bool frame_error;
    bool eot;

    while (true)
    {
        frame_size = recvfrom(socket_fd, (char *)frame, MAX_FRAME_SIZE,
                              MSG_WAITALL, (struct sockaddr *)&server_addr,
                              &server_addr_size);
        cout << "[Packet Received]\n";

        frame_error = read_frame(&recv_seq_num, data, &data_size, &eot, frame);

        create_ack(recv_seq_num, ack, frame_error);
        sendto(socket_fd, ack, ACK_SIZE, 0,
               (const struct sockaddr *)&server_addr, sizeof(server_addr));
        cout << "[Packet Sent]\n";
    }
}

string get_filename_ext(char *fname)
{

    // store the position of last '.' in the file name
    string file_name = fname;
    int position = file_name.find_last_of(".");

    // store the characters after the '.' from the file_name string
    string result = file_name.substr(position + 1);

    return result;
}

int main(int argc, char *argv[])
{
    char *ip;
    int port;
    int window_len = 5;
    int max_buffer_size = MAX_DATA_SIZE * 1024;
    struct hostent *dest_hnet;
    char *fname;
    char input[5000];

    if (argc == 4)
    {
        fname = argv[1];
        ip = argv[2];
        port = atoi(argv[3]);
    }
    else
    {
        cerr << "usage: ./client <filename> <ip> <port>" << endl;
        return 1;
    }

    dest_hnet = gethostbyname(ip);
    if (!dest_hnet)
    {
        cerr << "unknown host: " << ip << endl;
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    bcopy(dest_hnet->h_addr, (char *)&server_addr.sin_addr, dest_hnet->h_length);
    server_addr.sin_port = htons(port);

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    if (::bind(socket_fd, (const struct sockaddr *)&client_addr,
               sizeof(client_addr)) < 0)
    {
        cerr << "socket binding failed" << endl;
        return 1;
    }

    sendto(socket_fd, fname, strlen(fname) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        cout << "[Packet Sent]\n";
    recvfrom(socket_fd, input, 5000, 0, (struct sockaddr *)&server_addr, &server_addr_size);
        cout << "[Packet Received]\n";

    // Creating a file.
    string ext = get_filename_ext(fname);

    string output = (char *)"output";
    string temp_name_with_extension = output;
    temp_name_with_extension.push_back('.');
    temp_name_with_extension.append(ext);
    const char *tempfname = temp_name_with_extension.c_str();

    char *name_with_extension = (char *)tempfname;

    // PROGRAM
    FILE *file = file = fopen(name_with_extension, "wb");
    char buffer[max_buffer_size];
    int buffer_size;

    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frame_size;
    int data_size;
    int lfr, laf;
    int recv_seq_num;
    bool eot;
    bool frame_error;

    /* Receive frames until EOT */
    bool recv_done = false;
    int buffer_num = 0;
    while (!recv_done)
    {
        buffer_size = max_buffer_size;
        memset(buffer, 0, buffer_size);

        int recv_seq_count = (int)max_buffer_size / MAX_DATA_SIZE;
        bool window_recv_mask[window_len];
        for (int i = 0; i < window_len; i++)
        {
            window_recv_mask[i] = false;
        }
        lfr = -1;
        laf = lfr + window_len;

        /* Receive current buffer with sliding window */
        while (true)
        {
            frame_size = recvfrom(socket_fd, (char *)frame, MAX_FRAME_SIZE,
                                  MSG_WAITALL, (struct sockaddr *)&server_addr,
                                  &server_addr_size);
        cout << "[Packet Received]\n";

            frame_error = read_frame(&recv_seq_num, data, &data_size, &eot, frame);

            create_ack(recv_seq_num, ack, frame_error);
            sendto(socket_fd, ack, ACK_SIZE, 0,
                   (const struct sockaddr *)&server_addr, sizeof(server_addr));
        cout << "[Packet Sent]\n";

            if (recv_seq_num <= laf)
            {
                if (!frame_error)
                {
                    int buffer_shift = recv_seq_num * MAX_DATA_SIZE;

                    if (recv_seq_num == lfr + 1)
                    {
                        memcpy(buffer + buffer_shift, data, data_size);

                        int shift = 1;
                        for (int i = 1; i < window_len; i++)
                        {
                            if (!window_recv_mask[i])
                                break;
                            shift += 1;
                        }
                        for (int i = 0; i < window_len - shift; i++)
                        {
                            window_recv_mask[i] = window_recv_mask[i + shift];
                        }
                        for (int i = window_len - shift; i < window_len; i++)
                        {
                            window_recv_mask[i] = false;
                        }
                        lfr += shift;
                        laf = lfr + window_len;
                    }
                    else if (recv_seq_num > lfr + 1)
                    {
                        if (!window_recv_mask[recv_seq_num - (lfr + 1)])
                        {
                            memcpy(buffer + buffer_shift, data, data_size);
                            window_recv_mask[recv_seq_num - (lfr + 1)] = true;
                        }
                    }

                    /* Set max sequence to sequence of frame with EOT */
                    if (eot)
                    {
                        buffer_size = buffer_shift + data_size;
                        recv_seq_count = recv_seq_num + 1;
                        recv_done = true;
                    }
                }
            }

            /* Move to next buffer if all frames in current buffer has been received */
            if (lfr >= recv_seq_count - 1)
                break;
        }
        fwrite(buffer, 1, buffer_size, file);
        buffer_num += 1;
    }

    fclose(file);

    /* Start thread to keep sending requested ack to sender for 3 seconds */
    thread stdby_thread(send_ack);
    time_stamp start_time = current_time();
    while (elapsed_time(current_time(), start_time) < STDBY_TIME)
    {
        cout << "\r"
             << "Wating for packet..." << flush;
        sleep_for(300);
    }
    stdby_thread.detach();

    cout << "\nDONE" << endl;

    return 0;
}