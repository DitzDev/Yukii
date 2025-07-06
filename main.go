package main

import (
	"context"
	"flag"
	"os"
	"os/signal"
	"syscall"
	"time"

	"yukii-bot/lib/config"
	"yukii-bot/lib/database"
	"yukii-bot/lib/logger"
	"yukii-bot/lib/whatsapp"
	"yukii-bot/plugins"
	_ "github.com/mattn/go-sqlite3"
)

var (
	devMode = flag.Bool("dev", false, "Run in development mode")
	qrMode  = flag.Bool("qr", false, "Use QR code for authentication")
	pairCode = flag.String("pair", "", "Phone number for pairing code authentication")
)

func main() {
	flag.Parse()

	logger.Init(*devMode)
	
	cfg, err := config.Load()
	if err != nil {
		logger.Fatal("Failed to load configuration", err)
	}
	
	db, err := database.Init(cfg.Database.Path)
	if err != nil {
		logger.Fatal("Failed to initialize database", err)
	}
	defer db.Close()
	
	client, err := whatsapp.NewClient(cfg, db)
	if err != nil {
		logger.Fatal("Failed to create WhatsApp client", err)
	}

	if *qrMode {
		client.SetAuthMode(whatsapp.AuthModeQR)
	} else if *pairCode != "" {
		client.SetAuthMode(whatsapp.AuthModePair)
		client.SetPairCode(*pairCode)
	}
	
	pluginManager := plugins.NewManager(client, db)
	
	if err := pluginManager.LoadPlugins(); err != nil {
		logger.Fatal("Failed to load plugins", err)
	}
	
	client.SetMessageHandler(pluginManager.HandleMessage)
	
	if err := client.Connect(); err != nil {
		logger.Fatal("Failed to connect to WhatsApp", err)
	}
	
	logger.Info("🚀 Yukii Bot is starting...")
	
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	select {
	case <-sigChan:
		logger.Info("📴 Shutting down gracefully...")
		cancel()
		
		client.Disconnect()

		time.Sleep(2 * time.Second)
		
		logger.Info("👋 Goodbye!")
		
	case <-ctx.Done():
		logger.Info("📴 Context cancelled, shutting down...")
	}
}