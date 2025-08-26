# production-practice-2nd-course
Used in development:
Visual studio 2022
Qt 6.9

## For the project to work you need:
Turn on:
Qt Core
Qt GUI
Qt Widgets

## How to use?
By default, the local mode is enabled, to connect via the network you need to start a local server, it is in another branch. 
You need to take the URL from the server, by default it will be http://localhost:3000/api. 
You can set the token yourself with rights or use those that already exist, for example:
"read-only-token": ["read"],
"editor-token": ["read", "create", "update"],
"admin-token": ["read", "create", "update", "delete", "sync"],
"super-admin-token": ["*"],
