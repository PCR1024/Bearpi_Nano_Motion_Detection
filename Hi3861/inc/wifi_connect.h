/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WIFI_CONNECT_H__
#define __WIFI_CONNECT_H__

#define SELECT_WIFI_SSID "XIAOXIN"
#define SELECT_WIFI_PASSWORD "W8574;n5"

int WifiConnect(const char *ssid, const char *psk);

#endif /* __WIFI_CONNECT_H__ */
