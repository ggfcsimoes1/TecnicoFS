# SO Project 2020-21
## CHANGELOG

| Author       | Date           | Description  |
|:------------- |:-------------| :-----|
| -------- | 29-11-2020 | **Created Repo** |
| Gus      | 29-10-2020 | <ul><li>Added unlockAll function (state.c), basically replaces the for cycle in the applyCommands function in main.</li><li>Fixed minor comments and improved function documentation.</li></ul><ul><li>*Experimenting with locks in order to figure out the root of the problem in the delete function. Crashes always occur when there's a creation right after a deletion.*</li> |    
| Miguel    | 29-10-2020     |  Added circular buffer, has bugs, still testing. |
| Miguel & Gus   | 03-11-2020     |  Fixed some circular buffer bugs, still deadlocks once in a while. |
| Miguel    | 11-11-2020     |  Still has syncronization problems, buffer still has bugs. |
| Miguel & Gus   | 12-11-2020     |  Syncronization problems gone, buffer still has bugs.  |
| Gus      | 12-11-2020 | Built runTests.sh, a simple testing script |
| Miguel & Gus | 13-11-2020 | Started 'move' command, almost done, improved code. |
| Gus | 14-11-2020 | Embelished code a little bit, removed some repetitions in code, added some comments |
| Miguel & Gus | 17-11-2020 | Fixed circular buffer flags; Fixed move's verifications. No longer has bugs. |
| Miguel & Gus | 18-11-2020 | **Finished the 2nd Requirement** |
| -------- | 23-11-2020 | **3rd Requirement out** |
| Miguel & Gus | 23-11-2020 | Created tfsMount, tfsCreate, tfsMove, tfsLookup, tfsDelete |
| Miguel & Gus | 25-11-2020 | Created tfsPrint, improved makefile. |

## TO-DO

# 2nd Requirement

  - [x] Fine sync
  - [x] Fix delete functionality
  - [x] Add concurrent acess to the buffer array
  - [x] Move command
  - [x] Shell script

# 3rd Requirement

  - [x] Mount socket
  - [x] tfsXXX Api implementation *Check tfsPrint*
  - [ ] Unmount socket
  - [ ] Embelish code