# SO Project 2020-21

##CHANGELOG
29-10-2020 / Gus
-Added unlockAll function (state.c), basically replaces the for cycle in the applyCommands function in main
-Fixed minor comments and improved function documentation

~Experimenting with locks in order to figure out the root of the problem in the delete function. Crashes always occur when there's a creation right after a deletion.

##TO-DO

[~]Fine sync
[ ]Fix delete functionality
[ ]Add concurrent acess to the buffer array
[ ]Move command
[ ]Shell script