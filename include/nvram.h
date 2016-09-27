/* Copyright 2013-2014 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NVRAM_H
#define __NVRAM_H

int nvram_format(void *nvram_image, uint32_t nvram_size);
int nvram_check(void *nvram_image, uint32_t nvram_size);
void nvram_reinit(void);
bool nvram_validate(void);

const char *nvram_query(const char *name);
bool nvram_query_is(const char *key, const char *value);

#endif /* __NVRAM_H */
