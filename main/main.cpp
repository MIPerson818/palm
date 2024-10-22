#include <iostream>
#include <memory>
#include <typeinfo>

#include "Palm.h"

bool inputCheck(int &input) {
  std::set<int> valid_inputs = {0, 1, 2, 3};
  if (valid_inputs.count(input) > 0)
    return true;
  return false;
}

void displayImageThread(CPalm &palm) {
  cv::Mat frame;
  cv::namedWindow("EasyPalm", cv::WINDOW_NORMAL);
  cv::resizeWindow("EasyPalm", 480, 640);
  while (true) {
    frame = palm.getFrameWithPoints();
    cv::imshow("EasyPalm", frame);
    int key = cv::waitKey(10);
    if (key == 27) {
      cv::destroyWindow("EasyPalm");
      break;
    }
  }
}

int main() {
  enum USER_INPUT { REGISTER, RECOGNIZE, DATABASE, QUIT };
  std::unique_ptr<CPalm> palm(new CPalm);
  int input = -1;

  // std::ref确保多线程共用同一个对象，保证线程安全
  std::thread display_thread(displayImageThread, std::ref(*palm));

  do {
    std::cout << "\n选择模式：[0]注册 [1]识别 [2]查看数据库 [3]退出"
              << std::endl;
    while (true) {
      std::cout << ">>";
      if (std::cin >> input) {
        if (inputCheck(input))
          break;
      }
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "\n模式选择无效，请重新输入" << std::endl;
    }
    USER_INPUT user_input = static_cast<USER_INPUT>(input);
    switch (user_input) {
    case USER_INPUT::REGISTER:
      palm->userRegister();
      break;
    case USER_INPUT::RECOGNIZE:
      palm->userRecognize();
      break;
    case USER_INPUT::DATABASE:
      palm->userDatabase();
      break;
    case USER_INPUT::QUIT:
      palm->userQuit();
      break;
    default:
      break;
    }
  } while (true);

  // 等待支线程执行完毕
  display_thread.join();

  return 0;
}