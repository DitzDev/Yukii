package whatsapp

import (
	"context"
	"fmt"
	"os"
	"regexp"
	"strings"
	"sync"
	"time"

	"yukii-bot/lib/config"
	"yukii-bot/lib/database"
	"yukii-bot/lib/logger"

	"go.mau.fi/whatsmeow"
	"go.mau.fi/whatsmeow/proto/waE2E"
	"go.mau.fi/whatsmeow/store/sqlstore"
	"go.mau.fi/whatsmeow/types"
	"go.mau.fi/whatsmeow/types/events"
	waLog "go.mau.fi/whatsmeow/util/log"
	"google.golang.org/protobuf/proto"
)

type AuthMode int

const (
	AuthModeAuto AuthMode = iota
	AuthModeQR
	AuthModePair
)

type MessageHandler func(*Message) error

type Client struct {
	client         *whatsmeow.Client
	config         *config.Config
	db             *database.Database
	authMode       AuthMode
	pairCode       string
	messageHandler MessageHandler
	eventHandlers  map[string]func(interface{})
	loginMutex     sync.RWMutex
	isConnecting   bool
}

type Message struct {
	ID        string
	From      types.JID
	To        types.JID
	Body      string
	Type      string
	IsGroup   bool
	GroupInfo *types.GroupInfo
	Sender    types.JID
	Timestamp time.Time
	IsFromMe  bool

	Raw *events.Message
}

func NewClient(cfg *config.Config, db *database.Database) (*Client, error) {
	dbLog := waLog.Noop
	if cfg.WhatsApp.LogLevel == "DEBUG" {
		dbLog = waLog.Stdout("Database", "DEBUG", true)
	}

	if err := os.MkdirAll(cfg.WhatsApp.SessionPath, 0755); err != nil {
		return nil, err
	}
	
	ctx := context.Background()
	container, err := sqlstore.New(ctx, "sqlite3", fmt.Sprintf("file:%s/session.db?_foreign_keys=on", cfg.WhatsApp.SessionPath), dbLog)
	if err != nil {
		return nil, err
	}

	deviceStore, err := container.GetFirstDevice(ctx)
	if err != nil {
		return nil, err
	}

	clientLog := waLog.Noop
	if cfg.WhatsApp.LogLevel == "DEBUG" {
		clientLog = waLog.Stdout("Client", "DEBUG", true)
	}
	
	client := whatsmeow.NewClient(deviceStore, clientLog)
	
	return &Client{
		client:        client,
		config:        cfg,
		db:            db,
		authMode:      AuthModeAuto,
		eventHandlers: make(map[string]func(interface{})),
	}, nil
}

func (c *Client) SetAuthMode(mode AuthMode) {
	c.authMode = mode
}

func (c *Client) SetPairCode(code string) {
	c.pairCode = code
}

func (c *Client) SetMessageHandler(handler MessageHandler) {
	c.messageHandler = handler
}

func (c *Client) Connect() error {
	c.loginMutex.Lock()
	defer c.loginMutex.Unlock()

	if c.isConnecting {
		return fmt.Errorf("already connecting")
	}

	c.client.AddEventHandler(c.handleEvent)

	if c.client.Store.ID == nil {
		c.isConnecting = true
		defer func() { c.isConnecting = false }()
		
		if err := c.login(); err != nil {
			return err
		}
	} else {
		return c.client.Connect()
	}
	
	return nil
}

func (c *Client) login() error {
	switch c.authMode {
	case AuthModeQR:
		return c.loginQR()
	case AuthModePair:
		return c.loginPair()
	default:
		return c.loginQR()
	}
}

func (c *Client) loginQR() error {
	qrChan, err := c.client.GetQRChannel(context.Background())
	if err != nil {
		return err
	}

	if err := c.client.Connect(); err != nil {
		return err
	}
	
	logger.Info("📱 Please scan the QR code below:")
	
	for evt := range qrChan {
		if evt.Event == "code" {
			fmt.Println(evt.Code)
		} else {
			logger.Info("QR Channel event: %s", evt.Event)
		}
	}
	
	return nil
}

func (c *Client) loginPair() error {
	if c.pairCode == "" {
		return fmt.Errorf("pair code is required")
	}
	
	loginChan := make(chan bool, 1)
	errorChan := make(chan error, 1)
	
	c.client.AddEventHandler(func(evt interface{}) {
		switch e := evt.(type) {
		case *events.Connected:
			logger.Info("🔗 Connected to WhatsApp")
			time.Sleep(2 * time.Second)
			if c.client.Store.ID != nil {
				logger.Info("✅ Successfully logged in!")
				select {
				case loginChan <- true:
				default:
				}
			}
		case *events.LoggedOut:
			logger.Info("❌ Logged out from WhatsApp")
			select {
			case loginChan <- false:
			default:
			}
		case *events.Disconnected:
			logger.Info("⚠️ Disconnected from WhatsApp")
		case *events.ConnectFailure:
			logger.Error("❌ Connection failed: %v", e.Reason)
			select {
			case errorChan <- fmt.Errorf("connection failed: %v", e.Reason):
			default:
			}
		}
	})
	
	if err := c.client.Connect(); err != nil {
		return fmt.Errorf("failed to connect: %v", err)
	}
	
	logger.Info("⏳ Waiting for connection...")
	time.Sleep(3 * time.Second)
	
	if c.client.Store.ID != nil {
		logger.Info("✅ Already authenticated!")
		return nil
	}

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	
	logger.Info("🔄 Starting pairing process...")
	code, err := c.client.PairPhone(ctx, c.pairCode, true, whatsmeow.PairClientChrome, "Chrome (Linux)")
	if err != nil {
		return fmt.Errorf("failed to pair phone: %v", err)
	}
	
	logger.Info("🔐 Pairing code: %s", code)
	logger.Info("⏳ Waiting for authentication... (this may take up to 5 minutes)")
	
	timeout := time.After(5 * time.Minute) 
	
	for {
		select {
		case success := <-loginChan:
			if success {
				logger.Info("✅ Login successful!")
				return nil
			} else {
				return fmt.Errorf("authentication failed - logged out")
			}
		case err := <-errorChan:
			return fmt.Errorf("connection error: %v", err)
		case <-timeout:
			return fmt.Errorf("authentication timeout - please try again")
		case <-time.After(15 * time.Second):
			if !c.client.IsConnected() {
				logger.Info("⚠️ Connection lost, reconnecting...")
				if err := c.client.Connect(); err != nil {
					logger.Error("Failed to reconnect: %v", err)
				}
			} else {
				logger.Info("⏳ Still waiting for authentication...")
			}
		}
	}
}

func (c *Client) handleEvent(evt interface{}) {
	switch e := evt.(type) {
	case *events.Message:
		c.handleMessage(e)
	case *events.Connected:
		logger.Connection("connected")
	case *events.Disconnected:
		logger.Connection("disconnected")
	case *events.LoggedOut:
		logger.Connection("logged out")
	case *events.ConnectFailure:
		logger.Error("Connection failure: %v", e.Reason)
	case *events.ClientOutdated:
		logger.Error("Client outdated - please update whatsmeow")
	}
}

func (c *Client) handleMessage(evt *events.Message) {
	if c.messageHandler == nil {
		return
	}
	
	if evt.Message == nil {
		return
	}
	
	msg := c.convertMessage(evt)
	
	sender := c.getDisplayName(msg.Sender)
	logger.MessageIn(sender, msg.Type, msg.Body, msg.Sender.String())
	
	if err := c.messageHandler(msg); err != nil {
		logger.Error("Failed to handle message: %v", err)
	}
}

func (c *Client) convertMessage(evt *events.Message) *Message {
	msg := &Message{
		ID:        evt.Info.ID,
		From:      evt.Info.Chat,
		Sender:    evt.Info.Sender,
		Timestamp: evt.Info.Timestamp,
		IsFromMe:  evt.Info.IsFromMe,
		IsGroup:   evt.Info.IsGroup,
		Raw:       evt,
	}
	
	if msg.IsGroup {
		msg.GroupInfo = &types.GroupInfo{
			JID: evt.Info.Chat,
		}
	}
	
	switch {
	case evt.Message.Conversation != nil:
		msg.Body = *evt.Message.Conversation
		msg.Type = "text"
	case evt.Message.ExtendedTextMessage != nil:
		msg.Body = *evt.Message.ExtendedTextMessage.Text
		msg.Type = "text"
	case evt.Message.ImageMessage != nil:
		msg.Body = "[Image]"
		if evt.Message.ImageMessage.Caption != nil {
			msg.Body = *evt.Message.ImageMessage.Caption
		}
		msg.Type = "image"
	case evt.Message.VideoMessage != nil:
		msg.Body = "[Video]"
		if evt.Message.VideoMessage.Caption != nil {
			msg.Body = *evt.Message.VideoMessage.Caption
		}
		msg.Type = "video"
	case evt.Message.AudioMessage != nil:
		msg.Body = "[Audio]"
		msg.Type = "audio"
	case evt.Message.DocumentMessage != nil:
		msg.Body = "[Document]"
		if evt.Message.DocumentMessage.Title != nil {
			msg.Body = *evt.Message.DocumentMessage.Title
		}
		msg.Type = "document"
	case evt.Message.StickerMessage != nil:
		msg.Body = "[Sticker]"
		msg.Type = "sticker"
	case evt.Message.LocationMessage != nil:
		msg.Body = "[Location]"
		msg.Type = "location"
	case evt.Message.ContactMessage != nil:
		msg.Body = "[Contact]"
		msg.Type = "contact"
	default:
		msg.Body = "[Unknown Message]"
		msg.Type = "unknown"
	}
	
	return msg
}

func (c *Client) getDisplayName(jid types.JID) string {
	if jid.Server == types.DefaultUserServer {
		ctx := context.Background()
		contact, err := c.client.Store.Contacts.GetContact(ctx, jid)
		if err == nil && contact.FullName != "" {
			return contact.FullName
		}
		return jid.User
	}
	
	return jid.User
}

func (c *Client) SendMessage(to types.JID, text string) error {
	msg := &waE2E.Message{
		Conversation: proto.String(text),
	}
	
	_, err := c.client.SendMessage(context.Background(), to, msg)
	if err != nil {
		return err
	}
	
	recipient := c.getDisplayName(to)
	logger.MessageOut(recipient, "text", text, to.String())
	
	return nil
}

func (c *Client) SendReply(original *Message, text string) error {
	msg := &waE2E.Message{
		ExtendedTextMessage: &waE2E.ExtendedTextMessage{
			Text: proto.String(text),
			ContextInfo: &waE2E.ContextInfo{
				StanzaID:    proto.String(original.ID),
				Participant: proto.String(original.Sender.String()),
			},
		},
	}
	
	_, err := c.client.SendMessage(context.Background(), original.From, msg)
	if err != nil {
		return err
	}
	
	recipient := c.getDisplayName(original.From)
	logger.MessageOut(recipient, "reply", text, original.From.String())
	
	return nil
}

func (c *Client) Disconnect() {
	c.loginMutex.Lock()
	defer c.loginMutex.Unlock()
	
	if c.client != nil {
		c.client.Disconnect()
	}
}

func (c *Client) GetJID() types.JID {
	if c.client.Store.ID == nil {
		return types.JID{}
	}
	return *c.client.Store.ID
}

func (c *Client) IsConnected() bool {
	return c.client.IsConnected()
}

func (c *Client) ConnectWithRetry(maxRetries int) error {
	var lastErr error
	
	for i := 0; i < maxRetries; i++ {
		if err := c.Connect(); err != nil {
			lastErr = err
			backoff := time.Duration(i+1) * time.Second * 2
			logger.Info("🔄 Retry %d/%d failed, waiting %v before next attempt", i+1, maxRetries, backoff)
			time.Sleep(backoff)
			continue
		}
		return nil
	}
	
	return fmt.Errorf("failed to connect after %d attempts: %v", maxRetries, lastErr)
}

func HasPrefix(body, prefix string) bool {
	return strings.HasPrefix(body, prefix)
}

func HasRichPrefix(body string) (bool, string) {
	re := regexp.MustCompile(`^([^\w\s]+)`)
	matches := re.FindStringSubmatch(body)
	if len(matches) > 1 {
		return true, matches[1]
	}
	return false, ""
}

func ExtractCommand(body, prefix string) (string, []string) {
	if HasPrefix(body, prefix) {
		body = strings.TrimPrefix(body, prefix)
	} else if hasRich, richPrefix := HasRichPrefix(body); hasRich {
		body = strings.TrimPrefix(body, richPrefix)
	}
	
	parts := strings.Fields(body)
	if len(parts) == 0 {
		return "", []string{}
	}
	
	command := strings.ToLower(parts[0])
	args := parts[1:]
	
	return command, args
}

func IsCommand(body, prefix string) bool {
	if HasPrefix(body, prefix) {
		return true
	}
	
	hasRich, _ := HasRichPrefix(body)
	return hasRich
}