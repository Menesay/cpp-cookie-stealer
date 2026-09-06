
### c++ windows cookie stealer

> AI generated

---

### directories
#### client
windows client versions

#### c2
python server script.

#### test-files
test files for server

#### old
archived files

#### prompts.txt
prompts used for generating the malware.
<br>
the main skeleton is created with making a "backup server"
<br>
then added various functions gradually

---

### infra

#### cookies
brave
chrome

#### server
dynamic c2 addresses : drive file name and youtube channel desc for gathering c2 address. (as fallback)

client -> cloudflare -> c2 -> apache reverse proxy -> python server

#### obfuscation
graph and math algorithms
compile-time xor on strings
runtime xor on strings
runtime windows api calls
pseudo random sleeps

#### trojanize
get requests to curseforge

#### persistence
up to version 7-4 : hkcu run registry

---

### compile
details at prompts.txt

