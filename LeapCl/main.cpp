//
//  main.cpp
//  LeapCl
//
//  Created by 一ノ瀬智太 on 2015/11/23.
//  Copyright © 2015年 ichinose.tomohiro. All rights reserved.
//

#include <iostream>
#include <string.h>
#include "../leapSDK/include/Leap.h"

//ソケット通信
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace Leap;

class SampleListener : public Listener {
public:
    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
    virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
    virtual void onDeviceChange(const Controller&);
    virtual void onServiceConnect(const Controller&);
    virtual void onServiceDisconnect(const Controller&);
    
private:
};

const std::string fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const std::string boneNames[] = {"Metacarpal", "Proximal", "Middle", "Distal"};
const std::string stateNames[] = {"STATE_INVALID", "STATE_START", "STATE_UPDATE", "STATE_END"};

void SampleListener::onInit(const Controller& controller) {
    //リスナーの初期化処理を行う。Leap::Controller::addListener()でリスナーを追加したときに呼び出される。
    std::cout << "Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller) {
    //Leap::ControllerクラスがLeap Motionセンサーと接続されたときに呼び出される。アプリケーション起動時にLeap Motionセンサーが接続されていない場合には、Leap Motionセンサーが接続されたときに呼ばれる。1台のPCに複数のLeap Motionセンサーを接続した場合には、先に接続された方が優先され、後に接続された方は特に通知もされず利用できないようだ。
    std::cout << "Connected" << std::endl;
    controller.enableGesture(Gesture::TYPE_CIRCLE);
    controller.enableGesture(Gesture::TYPE_KEY_TAP);
    controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
    controller.enableGesture(Gesture::TYPE_SWIPE);
}

void SampleListener::onDisconnect(const Controller& controller) {
    //Leap::ControllerクラスがLeap Motionセンサーから切断されたときに呼び出される。Leap Motionセンサーが物理的に引き抜かれた場合にも呼び出される
    // Note: not dispatched when running in a debugger.
    std::cout << "Disconnected" << std::endl;
}

void SampleListener::onExit(const Controller& controller) {
    //リスナーの終了処理を行う。Leap::Controller::removeListener()でリスナーを削除したときに呼び出される。
    std::cout << "Exited" << std::endl;
}

void SampleListener::onFrame(const Controller& controller) {
    //フレームのデータが更新されたときに呼び出される。Leap Motionセンサーのデータは全てここで処理される。
    // Get the most recent frame and report some basic information
    const Frame frame = controller.frame();
    std::cout << "Frame id: " << frame.id()
    << ", timestamp: " << frame.timestamp()
    << ", hands: " << frame.hands().count()
    << ", extended fingers: " << frame.fingers().extended().count()
    << ", tools: " << frame.tools().count()
    << ", gestures: " << frame.gestures().count() << std::endl;
    
    HandList hands = frame.hands();
    for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {
        // Get the first hand
        const Hand hand = *hl;
        std::string handType = hand.isLeft() ? "Left hand" : "Right hand";
        std::cout << std::string(2, ' ') << handType << ", id: " << hand.id()
        << ", palm position: " << hand.palmPosition() << std::endl;
        // Get the hand's normal vector and direction
        const Vector normal = hand.palmNormal();
        const Vector direction = hand.direction();
        
        // Calculate the hand's pitch, roll, and yaw angles
        std::cout << std::string(2, ' ') <<  "pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, "
        << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, "
        << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl;
        
        // Get the Arm bone
        Arm arm = hand.arm();
        std::cout << std::string(2, ' ') <<  "Arm direction: " << arm.direction()
        << " wrist position: " << arm.wristPosition()
        << " elbow position: " << arm.elbowPosition() << std::endl;
        
        // Get fingers
        const FingerList fingers = hand.fingers();
        for (FingerList::const_iterator fl = fingers.begin(); fl != fingers.end(); ++fl) {
            const Finger finger = *fl;
            std::cout << std::string(4, ' ') <<  fingerNames[finger.type()]
            << " finger, id: " << finger.id()
            << ", length: " << finger.length()
            << "mm, width: " << finger.width() << std::endl;
            
            // Get finger bones
            for (int b = 0; b < 4; ++b) {
                Bone::Type boneType = static_cast<Bone::Type>(b);
                Bone bone = finger.bone(boneType);
                std::cout << std::string(6, ' ') <<  boneNames[boneType]
                << " bone, start: " << bone.prevJoint()
                << ", end: " << bone.nextJoint()
                << ", direction: " << bone.direction() << std::endl;
            }
        }
    }
    
    // Get tools
    const ToolList tools = frame.tools();
    for (ToolList::const_iterator tl = tools.begin(); tl != tools.end(); ++tl) {
        const Tool tool = *tl;
        std::cout << std::string(2, ' ') <<  "Tool, id: " << tool.id()
        << ", position: " << tool.tipPosition()
        << ", direction: " << tool.direction() << std::endl;
    }
    
    // Get gestures
    const GestureList gestures = frame.gestures();
    for (int g = 0; g < gestures.count(); ++g) {
        Gesture gesture = gestures[g];
        
        switch (gesture.type()) {
            case Gesture::TYPE_CIRCLE:
            {
                CircleGesture circle = gesture;
                std::string clockwiseness;
                
                if (circle.pointable().direction().angleTo(circle.normal()) <= PI/2) {
                    clockwiseness = "clockwise";
                } else {
                    clockwiseness = "counterclockwise";
                }
                
                // Calculate angle swept since last frame
                float sweptAngle = 0;
                if (circle.state() != Gesture::STATE_START) {
                    CircleGesture previousUpdate = CircleGesture(controller.frame(1).gesture(circle.id()));
                    sweptAngle = (circle.progress() - previousUpdate.progress()) * 2 * PI;
                }
                std::cout << std::string(2, ' ')
                << "Circle id: " << gesture.id()
                << ", state: " << stateNames[gesture.state()]
                << ", progress: " << circle.progress()
                << ", radius: " << circle.radius()
                << ", angle " << sweptAngle * RAD_TO_DEG
                <<  ", " << clockwiseness << std::endl;
                break;
            }
            case Gesture::TYPE_SWIPE:
            {
                SwipeGesture swipe = gesture;
                std::cout << std::string(2, ' ')
                << "Swipe id: " << gesture.id()
                << ", state: " << stateNames[gesture.state()]
                << ", direction: " << swipe.direction()
                << ", speed: " << swipe.speed() << std::endl;
                break;
            }
            case Gesture::TYPE_KEY_TAP:
            {
                KeyTapGesture tap = gesture;
                std::cout << std::string(2, ' ')
                << "Key Tap id: " << gesture.id()
                << ", state: " << stateNames[gesture.state()]
                << ", position: " << tap.position()
                << ", direction: " << tap.direction()<< std::endl;
                break;
            }
            case Gesture::TYPE_SCREEN_TAP:
            {
                ScreenTapGesture screentap = gesture;
                std::cout << std::string(2, ' ')
                << "Screen Tap id: " << gesture.id()
                << ", state: " << stateNames[gesture.state()]
                << ", position: " << screentap.position()
                << ", direction: " << screentap.direction()<< std::endl;
                break;
            }
            default:
                std::cout << std::string(2, ' ')  << "Unknown gesture type." << std::endl;
                break;
        }
    }
    
    if (!frame.hands().isEmpty() || !gestures.isEmpty()) {
        std::cout << std::endl;
    }
}

void SampleListener::onFocusGained(const Controller& controller) {
    //既定ではLeap Motionのフレームデータはアプリケーションウィンドウがアクティブであるとき（＝最前面にあるとき）のみ通知される。onFocusGained()はアプリケーションがアクティブになったことを通知し、onFocusLost()はアプリケーションがアクティブでなくなったことを通知する。
    
    //通常のアプリケーションでは問題にならないが、タッチ入力をエミュレートする場合には、アプリケーションがアクティブでない場合がほとんどである（タッチ入力で別のアプリケーションを操作するため）。この設定はLeap::Controller::setPolicyFlags()で変更できる。setPolicyFlags()の引数にはPolicyFlag列挙体を与える。PolicyFlag列挙体には「POLICY_DEFAULT」と「POLICY_BACKGROUND_FRAMES」の2つが定義されている。POLICY_DEFAULTは既定のポリシーでアプリケーションがアクティブであるときのみフレームの更新が通知される。POLIC_BACKGROUND_FRAMESはバックグラウンド、つまりアプリケーションがアクティブではない状態でもフレームの更新が通知される。
    std::cout << "Focus Gained" << std::endl;
}

void SampleListener::onFocusLost(const Controller& controller) {
    //既定ではLeap Motionのフレームデータはアプリケーションウィンドウがアクティブであるとき（＝最前面にあるとき）のみ通知される。onFocusGained()はアプリケーションがアクティブになったことを通知し、onFocusLost()はアプリケーションがアクティブでなくなったことを通知する。
    
    //通常のアプリケーションでは問題にならないが、タッチ入力をエミュレートする場合には、アプリケーションがアクティブでない場合がほとんどである（タッチ入力で別のアプリケーションを操作するため）。この設定はLeap::Controller::setPolicyFlags()で変更できる。setPolicyFlags()の引数にはPolicyFlag列挙体を与える。PolicyFlag列挙体には「POLICY_DEFAULT」と「POLICY_BACKGROUND_FRAMES」の2つが定義されている。POLICY_DEFAULTは既定のポリシーでアプリケーションがアクティブであるときのみフレームの更新が通知される。POLIC_BACKGROUND_FRAMESはバックグラウンド、つまりアプリケーションがアクティブではない状態でもフレームの更新が通知される。
    std::cout << "Focus Lost" << std::endl;
}

void SampleListener::onDeviceChange(const Controller& controller) {
    //Leap Motionコントローラーの接続／切断が通知される。センサーが接続されているかどうかの判別に利用できる。
    std::cout << "Device Changed" << std::endl;
    const DeviceList devices = controller.devices();
    
    for (int i = 0; i < devices.count(); ++i) {
        std::cout << "id: " << devices[i].toString() << std::endl;
        std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
    }
}

void SampleListener::onServiceConnect(const Controller& controller) {
    //Leap Motionのデータを取得しているWindowsサービスとの接続／切断が通知される。onServiceDisconnect()が通知された場合には、何らかの原因でLeap Motionのデータが取得できなくなったので、サービスやOSの再起動が必要になる。
    std::cout << "Service Connected" << std::endl;
}

void SampleListener::onServiceDisconnect(const Controller& controller) {
    //Leap Motionのデータを取得しているWindowsサービスとの接続／切断が通知される。onServiceDisconnect()が通知された場合には、何らかの原因でLeap Motionのデータが取得できなくなったので、サービスやOSの再起動が必要になる。
    std::cout << "Service Disconnected" << std::endl;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, const char * argv[]) {
    // Create a sample listener and controller
    SampleListener listener;
    Controller controller;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[256];
    
    
    
      if (argc > 1 && strcmp(argv[1], "--bg") == 0)
        controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
    
    
    //while(1){
        portno = atoi("9999");//ポート番号
        sockfd = ::socket(AF_INET, SOCK_STREAM, 0);//ソケットの生成
        if (sockfd < 0)
            error("ERROR opening socket");
        server = gethostbyname("localhost");
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
            error("ERROR connecting");
        printf("Please enter the message: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        n = write(sockfd,buffer,strlen(buffer));//データの発信
        if (n < 0)
            error("ERROR writing to socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);//データの受信
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n",buffer);
    //}
    close(sockfd);
    // Have the sample listener receive events from the controller
    controller.addListener(listener);
    // Keep this process running until Enter is pressed
    std::cout << "Press Enter to quit..." << std::endl;
    std::cin.get();
    
    // Remove the sample listener when done
    controller.removeListener(listener);
    
    return 0;
}
