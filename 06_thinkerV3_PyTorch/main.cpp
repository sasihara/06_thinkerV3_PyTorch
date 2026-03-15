//
//  main.cpp
//  External thinker for othello version 2.00. 
//  

#include <iostream>
#include <WinSock2.h>		// Need to include before including "Windows.h", because it seems to include older version "winsock.h"
#include "common.h"
#include "externalThinkerMessages.hpp"
#include "main.hpp"
#include "think.hpp"
#include "messageGenerator.hpp"
#include "messageParser.hpp"
//#include "TFHandler.hpp"
#include "PTHHandler.hpp"
#include "history.hpp"

#pragma warning(disable:4996 6031 6305)

// Global
Logging logging;
History history;
Thinker thinker;

//
//	Function Name: main
//	Summary: Check commandline parameters, prepare the socket, wait for message and then handle the received message.
//
int main(int argc, char **argv)
{
    WSAData wsaData;
    SOCKET sock;
    struct sockaddr_in addr;
    struct sockaddr_in from;
    int sockaddr_in_size = sizeof(struct sockaddr_in);
    int port = 60001;
    char buf[4096];
    int messageLen;
    int ret;
    double spTemperature = SP_TEMPERATURE;
    bool limitTemperaturePeriod = false;
    int numIterations = PV_EVALUATE_COUNT;
    bool isBreadthFirst = false;
    RunningMode runningMode = RunningMode::RUNNINGMODE_AUTO;
    int gpuid = -1;
    bool forceGPU = true;

    // Logging
#ifdef _DEBUG
    LOGOUT_INIT(LOGLEVEL, "thinkerV3_log.txt");
#else
    LOGOUT_INIT(LOGLEVEL_WARNING, "thinkerV3_log.txt");
#endif

    // Parameter check
    try {
        if (argc >= 1) {
            for (int i = 1; i < argc; i++) {
                if (argv[i][0] == '-') {
                    switch (argv[i][1]) {
                    case 'p':
                        port = atoi(&argv[i][2]);
                        if (port <= 1024) {
                            throw - 1;
                        }
                        break;
                    case 'T':
                        spTemperature = atof(&argv[i][2]);
                        if (spTemperature < 0.0) throw - 2;
                        limitTemperaturePeriod = false;
                        break;
                    case 't':
                        spTemperature = atof(&argv[i][2]);
                        if (spTemperature < 0.0) throw - 2;
                        limitTemperaturePeriod = true;
                        break;
                    case 'i':
                        numIterations = atoi(&argv[i][2]);
                        if (numIterations <= 0) {
                            throw - 4;
                        }
                        break;
                    case 'b':
                        isBreadthFirst = true;
                        break;
                    case 'G':
                        runningMode = RunningMode::RUNNINGMODE_GPU;
                        if ('0' <= argv[i][2] && argv[i][2] <= '9') {
                            gpuid = atoi(&argv[i][2]);
                            if (gpuid < 0) throw - 5;
                        }
                        forceGPU = true;
                        break;
                    case 'g':
                        runningMode = RunningMode::RUNNINGMODE_GPU;
                        if ('0' <= argv[i][2] && argv[i][2] <= '9') {
                            gpuid = atoi(&argv[i][2]);
                            if (gpuid < 0) throw - 5;
                        }
                        forceGPU = false;
                        break;
                    case 'C':
                        runningMode = RunningMode::RUNNINGMODE_CPU;
                        break;
                    }
                }
                else {
                    throw - 3;
                }
            }
        }
    }
    catch(int ret){
        printf("\n\nUsage: thinkerV3.exe [options]\n\n");
        printf(" options([]:mandatory parameter, ():Optional Parameter)\n\n");
        printf("  -p[port number]: Port number to listen. ""port number"" must be larger than or equal to 1024.\n");
        printf("  -t[temperature]: Temperature (>= 0.0).\n");
        printf("  -b             : Breadth First search.\n");
        printf("  -C             : Force to use CPU.\n");
        printf("  -G(GPU)        : Force to use GPU of GPUID=(GPU).\n");
        printf("  -g(GPU)        : Try to use GPU of GPUID=(GPU). If impossible, use CPU.\n");
        printf("\n\n");

        return ret;
    }

    printf("\n");
    printf("%s\n", TEXTINFO);
    printf("Model = %s\n", thinker.getModelInfo());
    printf("Temperature = %f\n", spTemperature);
    printf("numIterations = %d\n", numIterations);
    printf("isBreadthFirst = %s\n", isBreadthFirst ? "true" : "false");
    printf("limitTemperaturePeriod = %s\n", limitTemperaturePeriod ? "true" : "false");
    gpuid < 0 ? printf("gpuid = (Not specify)\n") : printf("gpuid = %d\n", gpuid);
    printf("Force to use specified GPU = %s\n", forceGPU ? "true" : "false");
    printf("\n");

    logging.logout("***** Command Paramters ******");
    logging.logout("Port Number = %d", port);
    logging.logout("spTemperature = %f", spTemperature);
    logging.logout("numIterations = %d", numIterations);
    logging.logout("isBreadthFirst = %s", isBreadthFirst ? "true" : "false");
    logging.logout("limitTemperaturePeriod = %s", limitTemperaturePeriod ? "true" : "false");
    logging.logout("specified gpuid= %d", gpuid);
    logging.logout("Force to use specified GPU = %s", forceGPU ? "true" : "false");
    logging.logout("******************************");

    // 乱数の初期化
    srand((unsigned)time(0));

    // Thinkerの初期化
    ret = thinker.init(runningMode, spTemperature, numIterations, isBreadthFirst, limitTemperaturePeriod, gpuid, forceGPU);
    if (ret < 0) {
        return -4;
    }

    // Historyの初期化
    history.init();

    // Initialize winsock
    WSAStartup(MAKEWORD(2, 0), &wsaData);

	sock = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*) & addr, sizeof(addr));

    // Start to wait receiving requests
    printf("\nWaiting requests at port = %d...\n\n", port);

    // Receive and handle messages until QUIT message is received
    for (;;) {
        // 
        fd_set readfds;

        memset(buf, 0, sizeof(buf));

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        ret = select(1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock, &readfds)) {
            messageLen = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&from, &sockaddr_in_size);

            // Message length check
            if (messageLen < sizeof(MESSAGEHEADER)) continue;

            // Parse the received message
            MessageParser messageParser;
            if (messageParser.SetParam(buf, messageLen) != 0) continue;

            // Get the message type
            MESSAGETYPE messageType;
            if (messageParser.getMessageType(&messageType) != 0) continue;

            // Go out this loop if QUIT message is received
            if (messageType == MESSAGETYPE::QUIT) break;

            // Handle the received message according to the message type
            switch (messageType) {
            case MESSAGETYPE::INFORMATION_REQUEST:  // Information Request
                printf("Information Request Received.\n");
                HandleInformationRequest(messageParser, sock, from, sockaddr_in_size);
                break;
            case MESSAGETYPE::THINK_REQUEST:        // Think Request
                HandleThinkRequest(messageParser, sock, from, sockaddr_in_size);
                break;
            case MESSAGETYPE::THINK_STOP_REQUEST:   // Think Stop Request
                // Currently don't support this message.
                break;
            case MESSAGETYPE::GAME_FINISHED:        // Game Finished
                printf("Game Finished Received.\n");
                HandleGameFinished(messageParser, sock, from, sockaddr_in_size);
                break;
            default:
                break;
            }

            // Write to log
            LOGOUT_FLUSH();

            // Release the message
            messageParser.free();
        }
    }

    closesocket(sock);
    WSACleanup();

    // Logging
    LOGOUT_END();

    return 0;
}

//
//	Function Name: HandleInformationRequest
//	Summary: Handle Information Request message
//	
//	In:
//		messageParser       MessageParser instance of the received Information Request message
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      None
//
void HandleInformationRequest(MessageParser messageParser, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    char respMessage[MAX_MESSAGE_LENGTH];

    // Generate Information Response message
    int messageLength;
    MessageGenerator messageGenerator;
    messageGenerator.SetParams(respMessage, MAX_MESSAGE_LENGTH);
    messageGenerator.makeMessageHeader(MESSAGETYPE::INFORMATION_RESPONSE);
    messageGenerator.addTLVVersion(VERSION);

    char textInfo[1024];
    sprintf_s(textInfo, sizeof(textInfo), "%s(%s)", TEXTINFO, thinker.getModelInfo());
    messageGenerator.addTLVTextInfo(textInfo);

    // Check if building the message finished successfully
    if ((messageLength = messageGenerator.getSize()) < 0) return;

    // Send INFORMATION_RESPONSE to the peer
    sendto(sock, respMessage, messageLength, 0, (struct sockaddr*)&from, sockaddr_in_size);

    printf("Information Response sent.\n");
}

//
//	Function Name: HandleThinkRequest
//	Summary: Handle Think Request message
//	
//	In:
//		messageParser       MessageParser instance of the received Information Request message
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      None
//
void HandleThinkRequest(MessageParser messageParser, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    int tlvHead = sizeof(MESSAGEHEADER);       // tlvHead: TLV head pos to be processed
    DISKCOLORS board[64];
    int turn = 0;
    unsigned int id = 0;
    static GameId gameId;
    static bool prevGameIdAvail = false;
    int ret;

    // Get data from received message
    if (messageParser.getTLVParamsBoard(board) != 0) return;        // Get the board data
    if (messageParser.getTLVParamsTurn(&turn) != 0) return;         // Get the turn value
    if (messageParser.getTLVParamsID(&id) != 0) return;             // Get the ID value
    if (messageParser.getTLVParamsGameId(&gameId)) return;          // Get the game Id

    printf("Think Request Received. ID = %d.\n", id);

    // Send Think Accept message
    ret = SendThinkAccept(id, sock, from, sockaddr_in_size);

    printf("Think Accept sent. ID = %d. Thinking...\n", id);

    // Think
    int place;
    ret = thinker.think(turn, board, &place, gameId);

    if (ret == 0) {
        // Send Think Response message
        ret = SendThinkResponse(id, (unsigned char)place / 10, (unsigned char)place % 10, sock, from, sockaddr_in_size);

        printf("Think Response sent. ID = %d.\n", id);
    }
    else {
        ret = SendThinkReject(id, sock, from, sockaddr_in_size);

        printf("Think Reject sent. ID = %d.\n", id);
    }
}

//
//	Function Name: HandleGameFinished
//	Summary: Handle Game Finished message
//	
//	In:
//		messageParser       MessageParser instance of the received Information Request message
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      None
//
void HandleGameFinished(MessageParser messageParser, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    GameId gameId;
    RESULT result;
    DISKCOLORS diskcolor;
    int ret;

    // Get data from received message
    if (messageParser.getTLVParamsGameId(&gameId)) return;          // Get the game Id
    if (messageParser.getTLVParamsResult(&result)) return;          // Get the winner
    if (messageParser.getTLVParamsDiskColor(&diskcolor)) return;          // Get the winner

    // value値をセット & 学習データのファイル出力を行う
    // resultが自身なら1.0、相手なら-1.0, 引き分けなら0.0
    switch (result) {
    case RESULT::WIN :
        ret = history.finish(gameId, diskcolor, 1.0);
        break;
    case RESULT::LOSE :
        ret = history.finish(gameId, diskcolor, -1.0);
        break;
    case RESULT::EVEN :
        ret = history.finish(gameId, diskcolor, 0.0);
        break;
    default:
        break;
    }
    return;
}

//
//	Function Name: SendThinkAccept
//	Summary: Send Think Accept message
//	
//	In:
//		id                  Transaction ID received in Think Request message
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      0                   Succeed
//      -1                  Failed to build Think Accept message
//
int SendThinkAccept(int id, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    char message[MAX_MESSAGE_LENGTH];
    MessageGenerator messageGenerator;

    // Build Think Accpest message
    messageGenerator.SetParams(message, sizeof(message));
    messageGenerator.makeMessageHeader(MESSAGETYPE::THINK_ACCEPT);
    messageGenerator.addTLVID(id);

    // Check if building the message has succeeded or not
    if (messageGenerator.getSize() <= 0) return -1;

    // Send the message
    sendto(sock, message, messageGenerator.getSize(), 0, (struct sockaddr*)&from, sockaddr_in_size);

    return 0;
}

//
//	Function Name: SendThinkResponse
//	Summary: Send Think Response message
//	
//	In:
//		id                  Transaction ID received in Think Request message
//      x                   The place to put the disk in x-axis
//      y                   The place to put the disk in y-axis
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      0                   Succeed
//      -1                  Failed to build Think Response message
//
int SendThinkResponse(int id, unsigned char x, unsigned char y, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    char message[MAX_MESSAGE_LENGTH];
    MessageGenerator messageGenerator;

    // Build Think Response message
    messageGenerator.SetParams(message, sizeof(message));
    messageGenerator.makeMessageHeader(MESSAGETYPE::THINK_RESPONSE);
    messageGenerator.addTLVID(id);
    messageGenerator.addTLVPlace(x, y);

    // Check if building the message has succeeded or not
    if (messageGenerator.getSize() <= 0) return -1;

    // Send message
    sendto(sock, message, messageGenerator.getSize(), 0, (struct sockaddr*)&from, sockaddr_in_size);

    return 0;
}

//
//	Function Name: SendThinkReject
//	Summary: Send Think Reject message
//	
//	In:
//		id                  Transaction ID received in Think Request message
//      sock                socket to send the response message
//      from                The parameter to set the 5th parameter in sendto function to transmit the response message.  
//      sockaddr_in_size    The parameter to set the 6th parameter in sendto function to transmit the response message.
//
//	Return:
//      0                   Succeed
//      -1                  Failed to build Think Accept message
//
int SendThinkReject(int id, SOCKET sock, struct sockaddr_in from, int sockaddr_in_size)
{
    char message[MAX_MESSAGE_LENGTH];
    MessageGenerator messageGenerator;

    // Build Think Response message
    messageGenerator.SetParams(message, sizeof(message));
    messageGenerator.makeMessageHeader(MESSAGETYPE::THINK_REJECT);
    messageGenerator.addTLVID(id);

    // Check if building the message has succeeded or not
    if (messageGenerator.getSize() <= 0) return -1;

    // Send message
    sendto(sock, message, messageGenerator.getSize(), 0, (struct sockaddr*)&from, sockaddr_in_size);

    return 0;
}