package database

import (
	"encoding/json"
	"os"
	"path/filepath"
	"sync"

	"github.com/tidwall/gjson"
	"github.com/tidwall/sjson"
)

type Database struct {
	path string
	data map[string]interface{}
	mu   sync.RWMutex
}

func Init(path string) (*Database, error) {
	db := &Database{
		path: path,
		data: make(map[string]interface{}),
	}

	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return nil, err
	}

	if _, err := os.Stat(path); err == nil {
		if err := db.load(); err != nil {
			return nil, err
		}
	} else {
		if err := db.save(); err != nil {
			return nil, err
		}
	}
	
	return db, nil
}

func (db *Database) load() error {
	db.mu.Lock()
	defer db.mu.Unlock()
	
	data, err := os.ReadFile(db.path)
	if err != nil {
		return err
	}
	
	if len(data) == 0 {
		db.data = make(map[string]interface{})
		return nil
	}
	
	return json.Unmarshal(data, &db.data)
}

func (db *Database) save() error {
	data, err := json.MarshalIndent(db.data, "", "  ")
	if err != nil {
		return err
	}
	
	return os.WriteFile(db.path, data, 0644)
}

func (db *Database) Set(key string, value interface{}) error {
	db.mu.Lock()
	defer db.mu.Unlock()
	
	jsonData, err := json.Marshal(db.data)
	if err != nil {
		return err
	}
	
	jsonData, err = sjson.SetBytes(jsonData, key, value)
	if err != nil {
		return err
	}
	
	if err := json.Unmarshal(jsonData, &db.data); err != nil {
		return err
	}
	
	return db.save()
}

func (db *Database) Get(key string) gjson.Result {
	db.mu.RLock()
	defer db.mu.RUnlock()
	
	jsonData, err := json.Marshal(db.data)
	if err != nil {
		return gjson.Result{}
	}
	
	return gjson.GetBytes(jsonData, key)
}

func (db *Database) Delete(key string) error {
	db.mu.Lock()
	defer db.mu.Unlock()
	
	jsonData, err := json.Marshal(db.data)
	if err != nil {
		return err
	}
	
	jsonData, err = sjson.DeleteBytes(jsonData, key)
	if err != nil {
		return err
	}
	
	if err := json.Unmarshal(jsonData, &db.data); err != nil {
		return err
	}
	
	return db.save()
}

func (db *Database) Has(key string) bool {
	return db.Get(key).Exists()
}

func (db *Database) GetAll() map[string]interface{} {
	db.mu.RLock()
	defer db.mu.RUnlock()

	result := make(map[string]interface{})
	for k, v := range db.data {
		result[k] = v
	}
	
	return result
}

func (db *Database) Clear() error {
	db.mu.Lock()
	defer db.mu.Unlock()
	
	db.data = make(map[string]interface{})
	return db.save()
}

func (db *Database) Close() error {
	db.mu.Lock()
	defer db.mu.Unlock()
	
	return db.save()
}

func (db *Database) GetString(key string) string {
	return db.Get(key).String()
}

func (db *Database) GetInt(key string) int64 {
	return db.Get(key).Int()
}

func (db *Database) GetFloat(key string) float64 {
	return db.Get(key).Float()
}

func (db *Database) GetBool(key string) bool {
	return db.Get(key).Bool()
}

func (db *Database) GetArray(key string) []gjson.Result {
	return db.Get(key).Array()
}

func (db *Database) GetMap(key string) map[string]gjson.Result {
	return db.Get(key).Map()
}

func (db *Database) SetUser(jid, key string, value interface{}) error {
	return db.Set("users."+jid+"."+key, value)
}

func (db *Database) GetUser(jid, key string) gjson.Result {
	return db.Get("users." + jid + "." + key)
}

func (db *Database) HasUser(jid string) bool {
	return db.Has("users." + jid)
}

func (db *Database) SetGroup(jid, key string, value interface{}) error {
	return db.Set("groups."+jid+"."+key, value)
}

func (db *Database) GetGroup(jid, key string) gjson.Result {
	return db.Get("groups." + jid + "." + key)
}

func (db *Database) HasGroup(jid string) bool {
	return db.Has("groups." + jid)
}