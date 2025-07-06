package logger

import (
	"fmt"
	"strings"
	"time"

	"github.com/fatih/color"
)

var (
	devMode bool
	
	// Colors
	colorInfo    = color.New(color.FgCyan)
	colorSuccess = color.New(color.FgGreen)
	colorWarning = color.New(color.FgYellow)
	colorError   = color.New(color.FgRed)
	colorDebug   = color.New(color.FgMagenta)
	colorTime    = color.New(color.FgBlue)
	colorBold    = color.New(color.Bold)
)

func Init(dev bool) {
	devMode = dev
}

func timestamp() string {
	return time.Now().Format("15:04:05")
}

func formatMessage(level, message string, args ...interface{}) string {
	ts := colorTime.Sprintf("[%s]", timestamp())
	
	var levelColor *color.Color
	switch strings.ToUpper(level) {
	case "INFO":
		levelColor = colorInfo
	case "SUCCESS":
		levelColor = colorSuccess
	case "WARN", "WARNING":
		levelColor = colorWarning
	case "ERROR":
		levelColor = colorError
	case "DEBUG":
		levelColor = colorDebug
	default:
		levelColor = colorInfo
	}
	
	levelStr := levelColor.Sprintf("[%s]", strings.ToUpper(level))
	
	if len(args) > 0 {
		message = fmt.Sprintf(message, args...)
	}
	
	return fmt.Sprintf("%s %s %s", ts, levelStr, message)
}

func Info(message string, args ...interface{}) {
	fmt.Println(formatMessage("INFO", message, args...))
}

func Success(message string, args ...interface{}) {
	fmt.Println(formatMessage("SUCCESS", message, args...))
}

func Warning(message string, args ...interface{}) {
	fmt.Println(formatMessage("WARN", message, args...))
}

func Error(message string, args ...interface{}) {
	fmt.Println(formatMessage("ERROR", message, args...))
}

func Debug(message string, args ...interface{}) {
	if devMode {
		fmt.Println(formatMessage("DEBUG", message, args...))
	}
}

func Fatal(message string, err error) {
	if err != nil {
		Error("%s: %v", message, err)
	} else {
		Error(message)
	}
	panic(message)
}

func MessageIn(from, messageType, content string, jid string) {
	emoji := getMessageEmoji(messageType)
	fromStr := colorBold.Sprintf("<%s>", from)
	jidStr := colorDebug.Sprintf("(%s)", jid)
	
	contentPreview := content
	if len(contentPreview) > 50 {
		contentPreview = contentPreview[:50] + "..."
	}
	
	Info("📥 %s %s %s %s: %s", emoji, fromStr, jidStr, messageType, contentPreview)
}

func MessageOut(to, messageType, content string, jid string) {
	emoji := getMessageEmoji(messageType)
	toStr := colorBold.Sprintf("<%s>", to)
	jidStr := colorDebug.Sprintf("(%s)", jid)
	
	contentPreview := content
	if len(contentPreview) > 50 {
		contentPreview = contentPreview[:50] + "..."
	}
	
	Info("📤 %s %s %s %s: %s", emoji, toStr, jidStr, messageType, contentPreview)
}

func getMessageEmoji(messageType string) string {
	switch strings.ToLower(messageType) {
	case "text":
		return "💬"
	case "image":
		return "🖼️"
	case "video":
		return "🎥"
	case "audio":
		return "🎵"
	case "document":
		return "📄"
	case "sticker":
		return "🌟"
	case "location":
		return "📍"
	case "contact":
		return "👤"
	default:
		return "📱"
	}
}

func PluginLoaded(name string) {
	Success("🔌 Plugin loaded: %s", name)
}

func PluginExecuted(name, user string) {
	Info("⚡ Plugin executed: %s by %s", name, user)
}

func Connection(status string) {
	if status == "connected" {
		Success("🟢 WhatsApp connected")
	} else if status == "disconnected" {
		Warning("🔴 WhatsApp disconnected")
	} else {
		Info("🟡 WhatsApp %s", status)
	}
}