#!/usr/bin/env python3
"""
Analyze Issue #138 validation test results
Checks for common failure indicators in partition/healing scenarios
"""

import sys
import os
import csv
import argparse
from pathlib import Path
from typing import Dict, List, Tuple

class Colors:
    """ANSI color codes for terminal output"""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

def load_csv(csv_path: Path) -> List[Dict]:
    """Load CSV file into list of dictionaries"""
    if not csv_path.exists():
        return []
    
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        return list(reader)

def check_bridge_count(data: List[Dict], scenario: str) -> Tuple[bool, List[str]]:
    """Check if bridge count is correct (should be 1 after healing)"""
    issues = []
    passed = True
    
    for row in data:
        # Look for healed/unified states
        if 'partition_count' in row and row['partition_count'] == '1':
            if 'active_bridge_count' in row:
                bridge_count = int(row['active_bridge_count'])
                if bridge_count > 1:
                    time = row.get('time', 'unknown')
                    issues.append(
                        f"  Time {time}s: {bridge_count} bridges active (expected 1)"
                    )
                    passed = False
                elif bridge_count == 0:
                    time = row.get('time', 'unknown')
                    issues.append(
                        f"  Time {time}s: No bridge active (expected 1)"
                    )
                    passed = False
    
    return passed, issues

def check_message_delivery(data: List[Dict], scenario: str) -> Tuple[bool, List[str]]:
    """Check if message delivery rate is high in unified mesh"""
    issues = []
    passed = True
    
    for row in data:
        # Look for unified mesh states
        if 'partition_count' in row and row['partition_count'] == '1':
            if 'message_delivery_rate' in row:
                try:
                    delivery_rate = float(row['message_delivery_rate'])
                    if delivery_rate < 0.95:  # Less than 95%
                        time = row.get('time', 'unknown')
                        issues.append(
                            f"  Time {time}s: Low delivery rate {delivery_rate:.2%} (expected >95%)"
                        )
                        passed = False
                except ValueError:
                    pass
    
    return passed, issues

def check_isolated_nodes(data: List[Dict], scenario: str) -> Tuple[bool, List[str]]:
    """Check if any nodes remain isolated after healing"""
    issues = []
    passed = True
    
    for row in data:
        # Look for healed states
        if 'partition_count' in row and row['partition_count'] == '1':
            if 'isolated_node_count' in row:
                try:
                    isolated = int(row['isolated_node_count'])
                    if isolated > 0:
                        time = row.get('time', 'unknown')
                        issues.append(
                            f"  Time {time}s: {isolated} nodes isolated (expected 0)"
                        )
                        passed = False
                except ValueError:
                    pass
            
            if 'unreachable_nodes' in row:
                try:
                    unreachable = int(row['unreachable_nodes'])
                    if unreachable > 0:
                        time = row.get('time', 'unknown')
                        issues.append(
                            f"  Time {time}s: {unreachable} nodes unreachable (expected 0)"
                        )
                        passed = False
                except ValueError:
                    pass
    
    return passed, issues

def analyze_scenario(csv_path: Path, scenario_name: str) -> bool:
    """Analyze a single scenario's results"""
    print(f"\n{Colors.BLUE}Analyzing: {scenario_name}{Colors.NC}")
    
    if not csv_path.exists():
        print(f"{Colors.RED}✗ FAIL: Results CSV not found{Colors.NC}")
        return False
    
    data = load_csv(csv_path)
    
    if not data:
        print(f"{Colors.RED}✗ FAIL: Empty or invalid CSV{Colors.NC}")
        return False
    
    print(f"  Loaded {len(data)} data points")
    
    # Run checks
    all_passed = True
    
    # Check 1: Bridge count
    bridge_passed, bridge_issues = check_bridge_count(data, scenario_name)
    if not bridge_passed:
        all_passed = False
        print(f"{Colors.RED}✗ Multiple bridges detected:{Colors.NC}")
        for issue in bridge_issues:
            print(issue)
    else:
        print(f"{Colors.GREEN}✓ Bridge count correct{Colors.NC}")
    
    # Check 2: Message delivery
    delivery_passed, delivery_issues = check_message_delivery(data, scenario_name)
    if not delivery_passed:
        all_passed = False
        print(f"{Colors.RED}✗ Low message delivery rate:{Colors.NC}")
        for issue in delivery_issues:
            print(issue)
    else:
        print(f"{Colors.GREEN}✓ Message delivery good{Colors.NC}")
    
    # Check 3: Isolated nodes
    isolated_passed, isolated_issues = check_isolated_nodes(data, scenario_name)
    if not isolated_passed:
        all_passed = False
        print(f"{Colors.RED}✗ Isolated nodes detected:{Colors.NC}")
        for issue in isolated_issues:
            print(issue)
    else:
        print(f"{Colors.GREEN}✓ No isolated nodes{Colors.NC}")
    
    if all_passed:
        print(f"{Colors.GREEN}✓ All checks PASSED{Colors.NC}")
    else:
        print(f"{Colors.RED}✗ Some checks FAILED{Colors.NC}")
    
    return all_passed

def main():
    parser = argparse.ArgumentParser(
        description="Analyze Issue #138 validation test results"
    )
    parser.add_argument(
        'results_dir',
        type=Path,
        help='Directory containing test results'
    )
    
    args = parser.parse_args()
    
    if not args.results_dir.exists():
        print(f"{Colors.RED}Error: Results directory not found: {args.results_dir}{Colors.NC}")
        return 1
    
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}Issue #138 Results Analysis{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"Results directory: {args.results_dir}")
    
    # Find all CSV files
    scenarios = {
        'rapid_partition_cycles': 'issue_138_rapid_partition_cycles',
        'message_routing': 'issue_138_message_routing',
        'uneven_partitions': 'issue_138_uneven_partitions',
        'stress_partition': 'issue_138_stress_partition',
        'cascade_healing': 'issue_138_cascade_healing',
    }
    
    passed_count = 0
    failed_count = 0
    
    for scenario_key, scenario_name in scenarios.items():
        # Look for metrics CSV
        csv_path = args.results_dir / scenario_name / 'metrics.csv'
        if not csv_path.exists():
            # Try alternative locations
            csv_path = args.results_dir / f'{scenario_name}.csv'
        
        if csv_path.exists():
            if analyze_scenario(csv_path, scenario_name):
                passed_count += 1
            else:
                failed_count += 1
        else:
            print(f"\n{Colors.YELLOW}⚠ SKIP: {scenario_name} (no results found){Colors.NC}")
    
    # Summary
    print(f"\n{Colors.BLUE}========================================{Colors.NC}")
    print(f"{Colors.BLUE}Analysis Summary{Colors.NC}")
    print(f"{Colors.BLUE}========================================{Colors.NC}")
    print(f"Scenarios Analyzed: {passed_count + failed_count}")
    print(f"{Colors.GREEN}Passed: {passed_count}{Colors.NC}")
    print(f"{Colors.RED}Failed: {failed_count}{Colors.NC}")
    
    if failed_count == 0:
        print(f"\n{Colors.GREEN}✓ All scenarios PASSED{Colors.NC}")
        print("Issue #138 is NOT present (or is fixed)")
        return 0
    else:
        print(f"\n{Colors.RED}✗ Some scenarios FAILED{Colors.NC}")
        print("Issue #138 MAY be present")
        print("\nCommon issue patterns detected:")
        print("- Multiple bridges after healing")
        print("- Low message delivery rates")
        print("- Isolated or unreachable nodes")
        return 1

if __name__ == '__main__':
    sys.exit(main())
