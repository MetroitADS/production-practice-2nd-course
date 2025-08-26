# Production Practice Project (Calendar)
A Qt-based application developed for practice.

## Development Environment
- **Visual studio 2022**
- **Qt 6.9**

## For the project to work you need:
### Turn on:
- **Qt Core**
- **Qt GUI**
- **Qt Widgets**

## How to use?

### Local
By default, the local mode is enabled.

### Network
To enable network function:
1. **Start server** (located ia a separate branch)
2. **Get the server URL** default: 'http://localhost:3000'
3. **Get the token** It is located in the same folder as the server in config.json
4. **Connect the server**

## Authentication Tokens
You can set your own tokens with rights or use ready-made ones. Here is an example of tokens:

| Token | Permissions | Description |
|-------|-------------|-------------|
| `read-only-token` | `["read"]` | Read-only access |
| `editor-token` | `["read", "create", "update"]` | Edit access |
| `admin-token` | `["read", "create", "update", "delete", "sync"]` | Administrative access |
| `super-admin-token` | `["*"]` | Full system access |
