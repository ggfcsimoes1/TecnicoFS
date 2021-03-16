# SO Project 2020-21

A simple filesystem with working datagram sockets and multithreaded support. 

## CHANGELOG

| Author       | Date           | Description  |
|:------------- |:-------------| :-----|
| Gus      | 29-10-2020 | <ul><li>Added unlockAll function (state.c), basically replaces the for cycle in the applyCommands function in main.</li><li>Fixed minor comments and improved function documentation.</li></ul><ul><li>*Experimenting with locks in order to figure out the root of the problem in the delete function. Crashes always occur when there's a creation right after a deletion.*</li> |    
| Miguel    | 29-10-2020     |  Added circular buffer (still testing)|
| Miguel & Gus   | 03-11-2020     |  Fixed circular buffer (sort of)|
| Miguel    | 11-11-2020     |  Still has syncronization problems|
| Miguel & Gus   | 12-11-2020     |  Syncronization problems gone  |
| Gus      | 12-11-2020 | Built runTests.sh, a simple testing script |
| Miguel & Gus | 13-11-2020 | Move almost done |
| Gus | 14-11-2020 | Embelished code a little bit, removed some repetitions in code, added some comments |
| Miguel & Gus | 17-11-2020 | Fixed circular buffer flags; Fixed move's verifications |
| Miguel & Gus | 18-11-2020 | GANDA PROJETO ACABADO MALTINHA |


## TO-DO

  - [x] Fine sync
  - [x] Fix delete functionality
  - [x] Add concurrent acess to the buffer array
  - [x] Move command
  - [x] Shell script
