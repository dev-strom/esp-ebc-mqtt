/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/

#include <Arduino.h>
#include <unity.h>

#include "command.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_dummy(void)
{
    TEST_ASSERT_EQUAL(1, 1);
}

void test_commands(void)
{
  TEST_ASSERT_EQUAL_STRING("fa0500000000000005f8", Command::CreateConnect().ToString().c_str());
  TEST_ASSERT_EQUAL_STRING("fa0600000000000006f8", Command::CreateDisconnect().ToString().c_str());
  TEST_ASSERT_EQUAL_STRING("fa0200000000000002f8", Command::CreateStop().ToString().c_str());
  TEST_ASSERT_EQUAL_STRING("fa21006401b4000a0af8", Command::CreateChargeCV(4.2, 1.0, 0.1).ToString().c_str());
}

// int main()
// {
//     UNITY_BEGIN();
//     RUN_TEST(test_dummy);
//     RUN_TEST(test_commands);
//     UNITY_END(); // stop unit testing

//     while (1)
//     {
//     }
// }


void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_dummy);
    RUN_TEST(test_commands);
    UNITY_END(); // stop unit testing
}

void loop() {}
