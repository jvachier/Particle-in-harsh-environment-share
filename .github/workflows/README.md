# CI/CD Pipeline Documentation

This repository uses GitHub Actions for automated building, testing, and deployment.

## Workflows

### 1. Main CI/CD Pipeline (`ci.yml`)

**Triggers:**
- Push to `main`, `develop`, or any `jv/*` branch
- Pull requests to `main` or `develop`
- Manual workflow dispatch

**Jobs:**

#### `build-macos`
- Builds both pragma and Metal implementations on macOS
- Uploads binaries as artifacts (7-day retention)
- Validates Metal GPU compilation

#### `build-linux`
- Builds pragma implementation on Ubuntu
- No Metal support (Linux doesn't have Metal framework)
- Ensures cross-platform compatibility

#### `lint`
- Runs cpplint for code quality checks
- Non-blocking (warnings only)

#### `test-quick`
- Quick validation test on macOS
- Runs Metal GPU binary for 60 seconds
- Ensures no crashes on startup

#### `benchmark` (manual trigger only)
- Performance comparison between pragma and Metal
- Only runs on `workflow_dispatch`
- Outputs timing comparisons

#### `release` (tags only)
- Creates GitHub releases when you push a tag like `v1.0.0`
- Packages binaries with config and tools
- Supports macOS (Metal + pragma) and Linux (pragma only)

### 2. Pull Request Check (`pr-check.yml`)

**Triggers:**
- Pull requests to `main`

**Jobs:**

#### `quick-check`
- Fast build verification on both macOS and Linux
- Matrix strategy for parallel testing
- Reports build status and binary sizes

#### `size-check`
- Monitors binary size changes
- Helps identify bloat

## Usage

### Running CI Manually

```bash
# Navigate to Actions tab on GitHub
# Click "CI/CD Pipeline" → "Run workflow"
# Select branch and click "Run workflow"
```

### Creating a Release

```bash
# Tag your commit
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# GitHub Actions will automatically:
# 1. Build all implementations
# 2. Package binaries
# 3. Create GitHub release with downloads
```

### Viewing Build Artifacts

```bash
# After any CI run:
# 1. Go to Actions tab
# 2. Click on your workflow run
# 3. Scroll to "Artifacts" section
# 4. Download binaries:
#    - main_pragma_macos
#    - main_metal_macos
#    - main_pragma_linux
```

## Status Badges

Add these to your README.md:

```markdown
![CI/CD](https://github.com/YOUR_USERNAME/Particle-in-harsh-environment-share/workflows/CI%2FCD%20Pipeline/badge.svg)
![PR Check](https://github.com/YOUR_USERNAME/Particle-in-harsh-environment-share/workflows/Pull%20Request%20Check/badge.svg)
```

## Platform Support

| Platform | Pragma SIMD | Metal GPU | CI Support |
|----------|-------------|-----------|------------|
| macOS    | ✅          | ✅        | ✅         |
| Linux    | ✅          | ❌        | ✅         |
| Windows  | ⚠️ (untested) | ❌      | ❌         |

## Troubleshooting

### Build fails on macOS
- Ensure libomp is installed: `brew install libomp`
- Check Xcode Command Line Tools: `xcode-select --install`

### Build fails on Linux
- Ensure g++-13 is installed: `sudo apt-get install g++-13`
- Check OpenMP support: `sudo apt-get install libomp-dev`

### Artifacts not appearing
- Check that workflow completed successfully
- Artifacts expire after 7 days
- Manual runs may not upload artifacts depending on configuration

## Local Testing

Before pushing, test locally:

```bash
# Test pragma implementation
cd implementation_pragma
make clean && make
./main_pragma.out

# Test Metal implementation (macOS only)
cd ../implementation_metal
make clean && make
./main_metal.out
```

## Performance Benchmarking

To run performance benchmarks in CI:

1. Go to Actions tab
2. Select "CI/CD Pipeline"
3. Click "Run workflow"
4. Select your branch
5. Run workflow

The benchmark job will output timing comparisons for pragma vs Metal GPU.
