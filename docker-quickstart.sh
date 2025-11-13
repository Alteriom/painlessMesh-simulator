#!/usr/bin/env bash
#
# Docker Quick Start Script for painlessMesh Simulator
# Usage: ./docker-quickstart.sh [build|dev|test|run]

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${CYAN}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed"
        echo "Install Docker: https://docs.docker.com/get-docker/"
        exit 1
    fi
    print_success "Docker is installed"
}

check_docker_compose() {
    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        print_error "Docker Compose is not installed"
        echo "Install Docker Compose: https://docs.docker.com/compose/install/"
        exit 1
    fi
    print_success "Docker Compose is installed"
}

# Main script
main() {
    print_header "painlessMesh Simulator - Docker Quick Start"
    
    # Check prerequisites
    check_docker
    check_docker_compose
    
    # Determine action
    ACTION=${1:-help}
    
    case "$ACTION" in
        build)
            print_header "Building Simulator"
            docker-compose build build
            docker-compose run --rm build
            print_success "Build complete! Artifacts in ./build/"
            ;;
        
        dev)
            print_header "Starting Development Container"
            echo "You'll get an interactive shell with full build environment"
            echo "Source code is mounted at /workspace"
            echo ""
            docker-compose run --rm dev
            ;;
        
        test)
            print_header "Running Tests"
            docker-compose run --rm test
            ;;
        
        run)
            print_header "Running Simulation"
            SCENARIO=${2:-examples/scenarios/simple_mesh.yaml}
            echo "Running scenario: $SCENARIO"
            echo ""
            docker-compose run --rm simulator painlessmesh-simulator \
                --config "$SCENARIO"
            ;;
        
        clean)
            print_header "Cleaning Up"
            docker-compose down -v
            docker system prune -f
            print_success "Cleanup complete"
            ;;
        
        help|*)
            echo ""
            echo "Usage: $0 [command]"
            echo ""
            echo "Commands:"
            echo "  build     Build the simulator in Docker"
            echo "  dev       Start interactive development container"
            echo "  test      Run tests in Docker"
            echo "  run       Run a simulation (optionally specify scenario)"
            echo "  clean     Clean up Docker containers and volumes"
            echo "  help      Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 build"
            echo "  $0 test"
            echo "  $0 run"
            echo "  $0 run examples/scenarios/stress_test.yaml"
            echo "  $0 dev"
            echo ""
            ;;
    esac
}

main "$@"
