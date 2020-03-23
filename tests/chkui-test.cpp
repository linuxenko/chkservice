#include <iostream>
#include <catch2/catch.hpp>
#include "chk-ui.h"

using namespace std;

TEST_CASE("should create window", "[ChkUI]") {
  MainWindow *win = new MainWindow();

//  REQUIRE(win->screenSize->w > 0);

  delete win;
}
