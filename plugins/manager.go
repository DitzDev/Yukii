package plugins

import (
	"fmt"
	"strings"

	"yukii-bot/lib/database"
	"yukii-bot/lib/logger"
	"yukii-bot/lib/whatsapp"
)

type PluginType int

const (
	PluginTypeBefore PluginType = iota
	PluginTypeCommand
	PluginTypeAll
	PluginTypeAfter
)

type Plugin interface {
	Name() string
	Description() string
	Usage() string
	Category() string
	Aliases() []string
	Execute(ctx *Context) error
}

type BasePlugin struct {
	PluginName        string
	PluginDescription string
	PluginUsage       string
	PluginCategory    string
	PluginAliases     []string
	NoPrefix          bool
	PluginType        PluginType
}

func (p *BasePlugin) Name() string        { return p.PluginName }
func (p *BasePlugin) Description() string { return p.PluginDescription }
func (p *BasePlugin) Usage() string       { return p.PluginUsage }
func (p *BasePlugin) Category() string    { return p.PluginCategory }
func (p *BasePlugin) Aliases() []string   { return p.PluginAliases }

type Context struct {
	Client    *whatsapp.Client
	Database  *database.Database
	Message   *whatsapp.Message
	Command   string
	Args      []string
	Body      string
	Prefix    string
	IsCommand bool
}

func (ctx *Context) Reply(text string) error {
	return ctx.Client.SendReply(ctx.Message, text)
}

func (ctx *Context) Send(text string) error {
	return ctx.Client.SendMessage(ctx.Message.From, text)
}

func (ctx *Context) GetArg(index int) string {
	if index >= 0 && index < len(ctx.Args) {
		return ctx.Args[index]
	}
	return ""
}

func (ctx *Context) GetArgs() []string {
	return ctx.Args
}

func (ctx *Context) GetBody() string {
	return ctx.Body
}

func (ctx *Context) IsGroup() bool {
	return ctx.Message.IsGroup
}

func (ctx *Context) GetSender() string {
	return ctx.Message.Sender.String()
}

func (ctx *Context) GetSenderUser() string {
	return ctx.Message.Sender.User
}

func (p *BasePlugin) Execute(ctx *Context) error {
    // TODO: Implement execute
	return nil
}

type Manager struct {
	client      *whatsapp.Client
	database    *database.Database
	plugins     map[string]Plugin
	beforePlugins []Plugin
	allPlugins    []Plugin
	afterPlugins  []Plugin
	prefix      string
}

func NewManager(client *whatsapp.Client, db *database.Database) *Manager {
	return &Manager{
		client:      client,
		database:    db,
		plugins:     make(map[string]Plugin),
		beforePlugins: []Plugin{},
		allPlugins:    []Plugin{},
		afterPlugins:  []Plugin{},
		prefix:      "!",
	}
}

func (m *Manager) LoadPlugins() error {
	m.registerPlugin(&PingPlugin{})
	//m.registerPlugin(&SpeedTestPlugin{})
	
	logger.Info("📦 Loaded %d plugins", len(m.plugins))
	return nil
}

func (m *Manager) registerPlugin(plugin Plugin) {
	name := strings.ToLower(plugin.Name())
	m.plugins[name] = plugin
	
	for _, alias := range plugin.Aliases() {
		m.plugins[strings.ToLower(alias)] = plugin
	}
	
	if basePlugin, ok := plugin.(*BasePlugin); ok {
		switch basePlugin.PluginType {
		case PluginTypeBefore:
			m.beforePlugins = append(m.beforePlugins, plugin)
		case PluginTypeAll:
			m.allPlugins = append(m.allPlugins, plugin)
		case PluginTypeAfter:
			m.afterPlugins = append(m.afterPlugins, plugin)
		}
	}
	
	logger.PluginLoaded(plugin.Name())
}

func (m *Manager) HandleMessage(msg *whatsapp.Message) error {
	if msg.IsFromMe {
		return nil
	}
	
	ctx := &Context{
		Client:   m.client,
		Database: m.database,
		Message:  msg,
		Body:     msg.Body,
		Prefix:   m.prefix,
	}
	
	if whatsapp.IsCommand(msg.Body, m.prefix) {
		ctx.IsCommand = true
		cmd, args := whatsapp.ExtractCommand(msg.Body, m.prefix)
		ctx.Command = cmd
		ctx.Args = args
	}
	
	for _, plugin := range m.beforePlugins {
		if err := plugin.Execute(ctx); err != nil {
			logger.Error("Before plugin %s failed: %v", plugin.Name(), err)
		}
	}
	
	if ctx.IsCommand && ctx.Command != "" {
		if plugin, exists := m.plugins[ctx.Command]; exists {
			if basePlugin, ok := plugin.(*BasePlugin); ok && basePlugin.NoPrefix {
				logger.PluginExecuted(plugin.Name(), ctx.GetSenderUser())
				if err := plugin.Execute(ctx); err != nil {
					logger.Error("Plugin %s failed: %v", plugin.Name(), err)
					return ctx.Reply(fmt.Sprintf("❌ Error: %v", err))
				}
			} else {
				logger.PluginExecuted(plugin.Name(), ctx.GetSenderUser())
				if err := plugin.Execute(ctx); err != nil {
					logger.Error("Plugin %s failed: %v", plugin.Name(), err)
					return ctx.Reply(fmt.Sprintf("❌ Error: %v", err))
				}
			}
		}
	}
	
	for _, plugin := range m.allPlugins {
		if err := plugin.Execute(ctx); err != nil {
			logger.Error("All plugin %s failed: %v", plugin.Name(), err)
		}
	}
	
	for _, plugin := range m.afterPlugins {
		if err := plugin.Execute(ctx); err != nil {
			logger.Error("After plugin %s failed: %v", plugin.Name(), err)
		}
	}
	
	return nil
}

func (m *Manager) GetPlugin(name string) (Plugin, bool) {
	plugin, exists := m.plugins[strings.ToLower(name)]
	return plugin, exists
}

func (m *Manager) GetPlugins() map[string]Plugin {
	return m.plugins
}

func (m *Manager) GetPluginsByCategory(category string) []Plugin {
	var plugins []Plugin
	for _, plugin := range m.plugins {
		if strings.EqualFold(plugin.Category(), category) {
			plugins = append(plugins, plugin)
		}
	}
	return plugins
}

func (m *Manager) SetPrefix(prefix string) {
	m.prefix = prefix
}