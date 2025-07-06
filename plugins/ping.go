package plugins

import (
	"fmt"
	"runtime"
	"time"
)

type PingPlugin struct {
	BasePlugin
}

func init() {
	// Auto-register plugin
	// This would be called by the plugin manager
}

func (p *PingPlugin) Name() string {
	return "Ping"
}

func (p *PingPlugin) Description() string {
	return "Check bot ping and system information"
}

func (p *PingPlugin) Usage() string {
	return "ping"
}

func (p *PingPlugin) Category() string {
	return "System"
}

func (p *PingPlugin) Aliases() []string {
	return []string{"p", "ping"}
}

func (p *PingPlugin) Execute(ctx *Context) error {
	start := time.Now()
	
	var m runtime.MemStats
	runtime.ReadMemStats(&m)
	
	responseTime := time.Since(start)
	//uptime := time.Since(start)
	
	memUsage := fmt.Sprintf("%.2f MB", float64(m.Alloc)/1024/1024)
	
	response := fmt.Sprintf(`🏓 *Pong!*

⏱️ *Response Time:* %v
💾 *Memory Usage:* %s
🔄 *Goroutines:* %d
🖥️ *OS:* %s
📊 *Architecture:* %s
🏃 *Go Version:* %s

✅ *Status:* Online and running!`, 
		responseTime,
		memUsage,
		runtime.NumGoroutine(),
		runtime.GOOS,
		runtime.GOARCH,
		runtime.Version())
	
	return ctx.Reply(response)
}