<div align="center">

# Yukii-Botz

### Yukii Botz | Open Source Project

<img src="media/Yukii.png" width="240" height="240" alt="Yukii">

</div>

<div align="center">

[![Open Source](https://badges.frapsoft.com/os/v1/open-source.svg?v=103)](https://github.com/ellerbrock/open-source-badges) [![Watchers](https://img.shields.io/github/watchers/DitzDev/Yukii.svg)](https://github.com/DitzDev/Yukii/watchers) [![Stars](https://img.shields.io/github/stars/DitzDev/Yukii.svg)](https://github.com/DitzDev/Yukii/stargazers) [![Forks](https://img.shields.io/github/forks/DitzDev/Yukii.svg)](https://github.com/DitzDev/Yukii/network/members) [![Repo Size](https://img.shields.io/github/repo-size/DitzDev/Yukii.svg)](https://github.com/DitzDev/Yukii) [![Issues](https://img.shields.io/github/issues/DitzDev/Yukii)](https://github.com/DitzDev/Yukii/issues)

<img src="https://raw.githubusercontent.com/andreasbm/readme/master/assets/lines/colored.png"/>

</div>

A powerful, modular WhatsApp bot built with Go and WhatsMeow library. Features plugin system, rich prefix support, and modern logging.

## Features

- **Modular Plugin System**: Automatically loads plugins from `plugins/` directory
- **Rich Prefix Support**: Detects special characters as prefixes using regex
- **No Prefix Mode**: Optional prefix-free commands for specific plugins
- **Before/All/After Hooks**: Execute plugins at different stages of message processing
- **Modern Logging**: Colorful, structured logging with message tracking
- **Flexible Authentication**: QR Code or Pairing Code authentication
- **Local Database**: JSON-based database similar to lowdb for easy data management
- **Docker Support**: Ready-to-use Docker configuration

## Quick Start

### Prerequisites

- Go 1.21 or higher
- Make (optional, for using Makefile)

### Installation

1. Clone the repository:
```bash
git clone https://github.com/yourusername/yukii-bot.git
cd yukii-bot
```

2. Install dependencies:
```bash
make install
```

3. Run the bot:
```bash
# Using QR Code (default)
make run

# Using QR Code explicitly
make run -- -qr

# Using Pairing Code
make run -- -pair +1234567890
```

### Using Docker

1. Build and run with Docker Compose:
```bash
docker-compose up -d
```

2. View logs:
```bash
docker-compose logs -f yukii-bot
```

## Authentication

### QR Code (Default)
```bash
go run main.go -qr
```
Scan the QR code with your WhatsApp app.

### Pairing Code
```bash
go run main.go -pair +1234567890 # + is optional
```
Enter the pairing code in your WhatsApp app.

## Creating Plugins

Create a new plugin in the `plugins/` directory:

```go
package plugins

import "fmt"

type MyPlugin struct {
    BasePlugin
}

func (p *MyPlugin) Name() string {
    return "MyPlugin"
}

func (p *MyPlugin) Description() string {
    return "My custom plugin"
}

func (p *MyPlugin) Usage() string {
    return "myplugin [args]"
}

func (p *MyPlugin) Category() string {
    return "Custom"
}

func (p *MyPlugin) Aliases() []string {
    return []string{"mp", "custom"}
}

func (p *MyPlugin) Execute(ctx *Context) error {
    return ctx.Reply("Hello from my plugin!")
}
```

## Built-in Plugins

### Ping Plugin
- **Command**: `!ping`, `!p`, `!pong`
- **Description**: Check bot ping and system information
- **Features**: Response time, memory usage, system info

## Plugin Features

### Rich Prefix Support
The bot automatically detects special characters as prefixes:
```
!ping     # Standard prefix
@ping     # Rich prefix
#ping     # Rich prefix
$ping     # Rich prefix
```

### No Prefix Mode
Set `NoPrefix: true` in your plugin to allow execution without prefix:
```go
type MyPlugin struct {
    BasePlugin
}

func init() {
    // This plugin can be executed without prefix
    plugin := &MyPlugin{
        BasePlugin: BasePlugin{
            PluginName: "MyPlugin",
            NoPrefix:   true,
        },
    }
}
```

### Plugin Types
Plugins can be executed at different stages:

```go
type MyPlugin struct {
    BasePlugin
}

func init() {
    plugin := &MyPlugin{
        BasePlugin: BasePlugin{
            PluginName: "MyPlugin",
            PluginType: PluginTypeBefore, // or PluginTypeAll, PluginTypeAfter
        },
    }
}
```

- **PluginTypeBefore**: Execute before command processing
- **PluginTypeCommand**: Normal command execution (default)
- **PluginTypeAll**: Execute for all messages
- **PluginTypeAfter**: Execute after command processing

## Database Usage

The bot uses a JSON-based database similar to lowdb:

```go
// In your plugin
func (p *MyPlugin) Execute(ctx *Context) error {
    // Set user data
    ctx.Database.SetUser(ctx.GetSender(), "points", 100)
    
    // Get user data
    points := ctx.Database.GetUser(ctx.GetSender(), "points").Int()
    
    // Set group data
    if ctx.IsGroup() {
        ctx.Database.SetGroup(ctx.Message.From.String(), "welcome_enabled", true)
    }
    
    // Generic database operations
    ctx.Database.Set("global.total_commands", 1000)
    totalCommands := ctx.Database.Get("global.total_commands").Int()
    
    return ctx.Reply(fmt.Sprintf("You have %d points", points))
}
```

## Available Make Commands

```bash
# Build the application
make build

# Run the application
make run

# Run in development mode
make dev

# Install dependencies
make install

# Clean build directory
make clean

# Run tests
make test

# Format code
make fmt

# Run go vet
make vet

# Run all checks
make check

# Build Docker image
make docker-build

# Run Docker container
make docker-run

# Show help
make help
```

## Security Features

- **User-based permissions**: Track users and their permissions
- **Group management**: Separate settings for groups
- **Command rate limiting**: Prevent spam (can be implemented in plugins)
- **Owner-only commands**: Restrict certain commands to bot owner

## Environment Variables

You can override configuration using environment variables:

```bash
export YUKII_BOT_NAME="MyBot"
export YUKII_BOT_PREFIX="."
export YUKII_DB_PATH="custom/path/db.json"
export YUKII_SESSION_PATH="custom/session"
```

## Advanced Usage

### Custom Message Handlers

```go
// In your plugin
func (p *MyPlugin) Execute(ctx *Context) error {
    // Check message type
    switch ctx.Message.Type {
    case "image":
        return ctx.Reply("Nice image!")
    case "video":
        return ctx.Reply("Cool video!")
    case "audio":
        return ctx.Reply("I can hear you!")
    }
    
    return nil
}
```

### Group-specific Features

```go
func (p *MyPlugin) Execute(ctx *Context) error {
    if ctx.IsGroup() {
        // Group-specific logic
        groupName := ctx.Message.From.String()
        return ctx.Reply(fmt.Sprintf("Hello group: %s", groupName))
    }
    
    // Private chat logic
    return ctx.Reply("Hello in private!")
}
```

### Rich Responses

```go
func (p *MyPlugin) Execute(ctx *Context) error {
    response := `🎉 *Welcome to Yukii Bot!*

📋 *Available Commands:*
• !ping - Check bot status
• !speedtest - Test internet speed
• !help - Show this help

🔗 *Links:*
• GitHub: https://github.com/yourusername/yukii-bot
• Documentation: https://docs.yourbot.com

💡 *Tip:* Use rich prefixes like @, #, $ instead of !`

    return ctx.Reply(response)
}
```

## Troubleshooting

### Common Issues

1. **"Failed to connect to WhatsApp"**
   - Check internet connection
   - Verify WhatsApp Web is not active on other devices
   - Try clearing session data: `rm -rf data/session`

2. **"Plugin not loading"**
   - Ensure plugin file is in `plugins/` directory
   - Check plugin implements all required methods
   - Verify no syntax errors: `go build`

3. **"Database permission denied"**
   - Ensure `data/` directory exists and is writable
   - Check file permissions: `chmod 755 data/`

### Debug Mode

Run with debug logging:
```bash
go run main.go -dev
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/new-feature`
3. Commit changes: `git commit -am 'Add new feature'`
4. Push to branch: `git push origin feature/new-feature`
5. Submit pull request

## Acknowledgments

- [WhatsMeow](https://github.com/tulir/whatsmeow) - WhatsApp Web API library
- [Fatih Color](https://github.com/fatih/color) - Terminal color library