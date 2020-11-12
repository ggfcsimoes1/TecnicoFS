# SO Project 2020-21
## CHANGELOG

| Author       | Date           | Description  |
|:------------- |:-------------| :-----|
| Gus      | 29-10-2020 | <ul><li>Added unlockAll function (state.c), basically replaces the for cycle in the applyCommands function in main.</li><li>Fixed minor comments and improved function documentation.</li></ul><ul><li>*Experimenting with locks in order to figure out the root of the problem in the delete function. Crashes always occur when there's a creation right after a deletion.*</li> |    
| Miguel    | 29-10-2020     |  Added circular buffer (still testing)|
| Miguel    | 03-11-2020     |  Fixed circular buffer (sort of)|
| Miguel    | 11-11-2020     |  Still has syncronization problems|
| Miguel    | 12-11-2020     |  Syncronization problems gone  |


## TO-DO

  - [x] Fine sync (Delete not working!)
  - [ ] Fix delete functionality
  - [ ] Add concurrent acess to the buffer array
  - [ ] Move command
  - [ ] Shell script
