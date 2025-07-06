package config

import (
	"encoding/json"
	"os"
	"path/filepath"
)

type Config struct {
	Bot struct {
		Name     string `json:"name"`
		Owner    string `json:"owner"`
		Prefix   string `json:"prefix"`
		Version  string `json:"version"`
	} `json:"bot"`
	
	Database struct {
		Path string `json:"path"`
	} `json:"database"`
	
	WhatsApp struct {
		SessionPath string `json:"session_path"`
		AutoReply   bool   `json:"auto_reply"`
		LogLevel    string `json:"log_level"`
	} `json:"whatsapp"`
	
	Plugins struct {
		Dir          string   `json:"dir"`
		AutoLoad     bool     `json:"auto_load"`
		DisabledList []string `json:"disabled_list"`
	} `json:"plugins"`
}

func Load() (*Config, error) {
	cfg := &Config{}
	
	cfg.Bot.Name = "Yukii"
	cfg.Bot.Owner = ""
	cfg.Bot.Prefix = "!"
	cfg.Bot.Version = "1.0.0"
	
	cfg.Database.Path = "data/database.json"
	
	cfg.WhatsApp.SessionPath = "data/session"
	cfg.WhatsApp.AutoReply = true
	cfg.WhatsApp.LogLevel = "INFO"
	
	cfg.Plugins.Dir = "plugins"
	cfg.Plugins.AutoLoad = true
	cfg.Plugins.DisabledList = []string{}
	
	configPath := "config.json"
	if _, err := os.Stat(configPath); os.IsNotExist(err) {
		os.MkdirAll("data", 0755)
		os.MkdirAll(cfg.Plugins.Dir, 0755)
		
		if err := Save(cfg, configPath); err != nil {
			return nil, err
		}
		return cfg, nil
	}
	
	data, err := os.ReadFile(configPath)
	if err != nil {
		return nil, err
	}
	
	if err := json.Unmarshal(data, cfg); err != nil {
		return nil, err
	}
	
	return cfg, nil
}

func Save(cfg *Config, path string) error {
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		return err
	}
	
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return err
	}
	
	return os.WriteFile(path, data, 0644)
}