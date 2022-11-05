// SERVER

#include <iostream>
#include <thread>
#include <mutex>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "helpers.h"

#define TIMEOUT 10

using namespace std;

int socket_fd;
struct sockaddr_in server_addr, client_addr;

int window_len = 5;
bool *window_ack_mask;
time_stamp *window_sent_time;
int lar, lfs;

time_stamp TMIN = current_time();
mutex window_info_mutex;

socklen_t client_addr_size = sizeof(client_addr);

void listen_ack()
{
    char ack[ACK_SIZE];
    int ack_size;
    int ack_seq_num;
    bool ack_error;
    bool ack_neg;

    while (true)
    {
        ack_size = recvfrom(socket_fd, (char *)ack, ACK_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &client_addr_size);
        cout << "[Packet Received]\n";

        ack_error = read_ack(&ack_seq_num, &ack_neg, ack);

        window_info_mutex.lock();

        if (!ack_error && ack_seq_num > lar && ack_seq_num <= lfs)
        {
            if (!ack_neg)
            {
                window_ack_mask[ack_seq_num - (lar + 1)] = true;
            }
            else
            {
                window_sent_time[ack_seq_num - (lar + 1)] = TMIN;
            }
        }

        window_info_mutex.unlock();
    }
}

int main(int argc, char *argv[])
{
    int port;
    int window_len = 5;
    int max_buffer_size = MAX_DATA_SIZE * 1024;
    char *fname = (char *)"";
    char input[5000];

    if (argc == 2)
    {
        port = atoi(argv[1]);
    }
    else
    {
        cerr << "usage: ./server <port>" << endl;
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    if (::bind(socket_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "socket binding failed" << endl;
        return 1;
    }

    recvfrom(socket_fd, input, 5000, 0, (struct sockaddr *)&client_addr, &client_addr_size);
        cout << "[Packet Received]\n";

    fname = input;

    sendto(socket_fd, input, 5000, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        cout << "[Packet Sent]\n";

    // PROGRAM
    if (access(fname, F_OK) == -1)
    {
        cerr << "file doesn't exist: " << fname << endl;
        return 1;
    }

    /* Open file to send */
    FILE *file = fopen(fname, "rb");
    char buffer[max_buffer_size];
    int buffer_size;

    /* Start thread to listen for ack */
    thread recv_thread(listen_ack);

    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    int frame_size;
    int data_size;

    /* Send file */
    bool read_done = false;
    int buffer_num = 0;
    while (!read_done)
    {

        /* Read part of file to buffer */
        buffer_size = fread(buffer, 1, max_buffer_size, file);
        if (buffer_size == max_buffer_size)
        {
            char temp[1];
            int next_buffer_size = fread(temp, 1, 1, file);
            if (next_buffer_size == 0)
                read_done = true;
            int error = fseek(file, -1, SEEK_CUR);
        }
        else if (buffer_size < max_buffer_size)
        {
            read_done = true;
        }

        window_info_mutex.lock();

        /* Initialize sliding window variables */
        int seq_count = buffer_size / MAX_DATA_SIZE + ((buffer_size % MAX_DATA_SIZE == 0) ? 0 : 1);
        int seq_num;
        window_sent_time = new time_stamp[window_len];
        window_ack_mask = new bool[window_len];
        bool window_sent_mask[window_len];
        for (int i = 0; i < window_len; i++)
        {
            window_ack_mask[i] = false;
            window_sent_mask[i] = false;
        }
        lar = -1;
        lfs = lar + window_len;

        window_info_mutex.unlock();

        /* Send current buffer with sliding window */
        bool send_done = false;
        while (!send_done)
        {

            window_info_mutex.lock();

            /* Check window ack mask, shift window if possible */
            if (window_ack_mask[0])
            {
                int shift = 1;
                for (int i = 1; i < window_len; i++)
                {
                    if (!window_ack_mask[i])
                        break;
                    shift += 1;
                }
                for (int i = 0; i < window_len - shift; i++)
                {
                    window_sent_mask[i] = window_sent_mask[i + shift];
                    window_ack_mask[i] = window_ack_mask[i + shift];
                    window_sent_time[i] = window_sent_time[i + shift];
                }
                for (int i = window_len - shift; i < window_len; i++)
                {
                    window_sent_mask[i] = false;
                    window_ack_mask[i] = false;
                }
                lar += shift;
                lfs = lar + window_len;
            }

            window_info_mutex.unlock();

            /* Send frames that has not been sent or has timed out */
            for (int i = 0; i < window_len; i++)
            {
                seq_num = lar + i + 1;

                if (seq_num < seq_count)
                {
                    window_info_mutex.lock();

                    if (!window_sent_mask[i] || (!window_ack_mask[i] && (elapsed_time(current_time(), window_sent_time[i]) > TIMEOUT)))
                    {
                        int buffer_shift = seq_num * MAX_DATA_SIZE;
                        data_size = (buffer_size - buffer_shift < MAX_DATA_SIZE) ? (buffer_size - buffer_shift) : MAX_DATA_SIZE;
                        memcpy(data, buffer + buffer_shift, data_size);

                        bool eot = (seq_num == seq_count - 1) && (read_done);
                        frame_size = create_frame(seq_num, frame, data, data_size, eot);

                        sendto(socket_fd, frame, frame_size, 0,
                               (const struct sockaddr *)&client_addr, sizeof(client_addr));
        cout << "[Packet Sent]\n";

                        window_sent_mask[i] = true;
                        window_sent_time[i] = current_time();
                    }

                    window_info_mutex.unlock();
                }
            }

            /* Move to next buffer if all frames in current buffer has been acked */
            if (lar >= seq_count - 1)
                send_done = true;
        }
        buffer_num += 1;
        if (read_done)
            break;
    }

    fclose(file);
    delete[] window_ack_mask;
    delete[] window_sent_time;
    recv_thread.detach();

    cout << "[DONE]" << endl;

    return 0;
}
