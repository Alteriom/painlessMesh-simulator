# Docker Quick Start Script for painlessMesh Simulator
# Usage: .\docker-quickstart.ps1 [build|dev|test|run|clean|help]

param(
    [Parameter(Position=0)]
    [string]$Action = "help",
    
    [Parameter(Position=1)]
    [string]$Scenario = "examples/scenarios/simple_mesh.yaml"
)

# Functions
function Write-ScriptHeader {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor Cyan
}

function Write-ScriptSuccess {
    param([string]$Message)
    Write-Host "* $Message" -ForegroundColor Green
}

function Write-ScriptError {
    param([string]$Message)
    Write-Host "X $Message" -ForegroundColor Red
}

function Test-Docker {
    if (!(Get-Command docker -ErrorAction SilentlyContinue)) {
        Write-ScriptError "Docker is not installed"
        Write-Host "Install Docker Desktop: https://docs.docker.com/desktop/install/windows-install/"
        exit 1
    }
    Write-ScriptSuccess "Docker is installed"
}

function Test-DockerCompose {
    $hasCompose = (docker compose version 2>$null) -or (Get-Command docker-compose -ErrorAction SilentlyContinue)
    if (!$hasCompose) {
        Write-ScriptError "Docker Compose is not available"
        Write-Host "Docker Compose should come with Docker Desktop"
        exit 1
    }
    Write-ScriptSuccess "Docker Compose is available"
}

# Main script
Write-ScriptHeader "painlessMesh Simulator - Docker Quick Start"

# Check prerequisites
Test-Docker
Test-DockerCompose

# Execute action
switch ($Action.ToLower()) {
    "build" {
        Write-ScriptHeader "Building Simulator"
        docker-compose build build
        docker-compose run --rm build
        Write-ScriptSuccess "Build complete! Artifacts in ./build/"
    }
    
    "dev" {
        Write-ScriptHeader "Starting Development Container"
        Write-Host "You will get an interactive shell with full build environment"
        Write-Host "Source code is mounted at /workspace"
        Write-Host ""
        docker-compose run --rm dev
    }
    
    "test" {
        Write-ScriptHeader "Running Tests"
        docker-compose run --rm test
    }
    
    "run" {
        Write-ScriptHeader "Running Simulation"
        Write-Host "Running scenario: $Scenario"
        Write-Host ""
        docker-compose run --rm simulator painlessmesh-simulator --config $Scenario
    }
    
    "clean" {
        Write-ScriptHeader "Cleaning Up"
        docker-compose down -v
        docker system prune -f
        Write-ScriptSuccess "Cleanup complete"
    }
    
    default {
        Write-Host ""
        Write-Host "Usage: .\docker-quickstart.ps1 [command]"
        Write-Host ""
        Write-Host "Commands:"
        Write-Host "  build     Build the simulator in Docker"
        Write-Host "  dev       Start interactive development container"
        Write-Host "  test      Run tests in Docker"
        Write-Host "  run       Run a simulation (optionally specify scenario)"
        Write-Host "  clean     Clean up Docker containers and volumes"
        Write-Host "  help      Show this help message"
        Write-Host ""
        Write-Host "Examples:"
        Write-Host "  .\docker-quickstart.ps1 build"
        Write-Host "  .\docker-quickstart.ps1 test"
        Write-Host "  .\docker-quickstart.ps1 run"
        Write-Host "  .\docker-quickstart.ps1 run examples/scenarios/stress_test.yaml"
        Write-Host "  .\docker-quickstart.ps1 dev"
        Write-Host ""
    }
}
