APP_NAME := yukii-bot
BUILD_DIR := build
MAIN_FILE := main.go

# Colors for output
GREEN := \033[0;32m
YELLOW := \033[0;33m
RED := \033[0;31m
NC := \033[0m # No Color

.PHONY: all build run clean deps install tidy dev help

all: build ## Build the application

build: ## Build the application
	@echo "$(GREEN)Building $(APP_NAME)...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@go build -o $(BUILD_DIR)/$(APP_NAME) $(MAIN_FILE)
	@echo "$(GREEN)Build completed: $(BUILD_DIR)/$(APP_NAME)$(NC)"

run: ## Run the application directly
	@echo "$(GREEN)Running $(APP_NAME)...$(NC)"
	@go run $(MAIN_FILE)

dev: ## Run in development mode (with auto-reload)
	@echo "$(GREEN)Starting development server...$(NC)"
	@go run $(MAIN_FILE) -dev

clean: ## Clean build directory
	@echo "$(YELLOW)Cleaning build directory...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)Clean completed$(NC)"

deps: ## Download dependencies
	@echo "$(GREEN)Downloading dependencies...$(NC)"
	@go mod download

install: deps ## Install dependencies and build
	@echo "$(GREEN)Installing dependencies...$(NC)"
	@go mod tidy
	@$(MAKE) build

tidy: ## Tidy up dependencies
	@echo "$(GREEN)Tidying dependencies...$(NC)"
	@go mod tidy

test: ## Run tests
	@echo "$(GREEN)Running tests...$(NC)"
	@go test -v ./...

fmt: ## Format code
	@echo "$(GREEN)Formatting code...$(NC)"
	@go fmt ./...

vet: ## Run go vet
	@echo "$(GREEN)Running go vet...$(NC)"
	@go vet ./...

check: fmt vet test ## Run all checks

docker-build: ## Build docker image
	@echo "$(GREEN)Building docker image...$(NC)"
	@docker build -t $(APP_NAME) .

docker-run: ## Run docker container
	@echo "$(GREEN)Running docker container...$(NC)"
	@docker run -it --rm $(APP_NAME)

help: ## Show this help
	@echo "$(GREEN)Available commands:$(NC)"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  $(YELLOW)%-15s$(NC) %s\n", $$1, $$2}'