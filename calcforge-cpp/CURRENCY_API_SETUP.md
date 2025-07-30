# CalcForge Currency API Setup Guide

## Overview
CalcForge supports live exchange rates through the exchangeratesapi.io service. By default, it uses high-precision fallback rates, but you can enable real-time rates by adding an API key.

## Current Status
- ✅ **Fallback Rates**: Working with 4+ decimal precision (e.g., 100 USD → 85.66 EUR)
- ⚙️ **Live Rates**: Available with API key setup
- 🔄 **Auto-Updates**: Hourly when API key is configured

## Quick Setup (3 Steps)

### Step 1: Get Your Free API Key
1. Visit: https://manage.exchangeratesapi.io/signup/free
2. Sign up for a free account (1,000 requests/month)
3. Copy your API key from the dashboard

### Step 2: Create API Key File
1. Navigate to your CalcForge.exe folder
2. Create a new text file named: `api_key.txt`
3. Paste your API key on the first line (no extra spaces)
4. Save the file

### Step 3: Restart CalcForge
- Close and restart CalcForge
- Check the logs to confirm: "API key loaded from: [path]"

## File Structure
```
CalcForge.exe
api_key.txt          ← Your API key file
worksheets.json
logs/
```

## Example api_key.txt
```
abc123def456789xyz
```
*Note: Replace with your actual API key*

## Verification
Check the latest log file in the `logs/` folder for:
- ✅ `API key loaded from: [path]`
- ✅ `Requesting exchange rate update from API`

If no API key file exists, you'll see:
- ℹ️ `No API key file found - using fallback rates only`
- ℹ️ `To enable live exchange rates, create api_key.txt`

## API Service Details
- **Provider**: exchangeratesapi.io (by APILayer)
- **Base URL**: https://api.exchangeratesapi.io/v1/
- **Update Frequency**: Every hour
- **Base Currency**: USD (all rates relative to US Dollar)
- **Supported Currencies**: 170+ world currencies
- **Free Tier**: 1,000 requests/month

## Pricing Plans
- **Free**: 1,000 requests/month
- **Starter**: $7.99/month - 5,000 requests
- **Basic**: $14.99/month - 10,000 requests
- **Pro**: $59.99/month - 100,000 requests

## Troubleshooting

### API Key Not Loading
- Ensure file is named exactly: `api_key.txt`
- Check file is in same folder as CalcForge.exe
- Verify API key has no extra spaces or characters
- Restart CalcForge after creating the file

### API Requests Failing
- Verify API key is valid in your dashboard
- Check internet connection
- Ensure you haven't exceeded monthly quota
- CalcForge will automatically fall back to static rates

### Rate Accuracy
- **With API**: Live rates updated hourly
- **Without API**: High-precision static rates (updated manually)
- Both provide professional-grade accuracy for calculations

## Security Notes
- API key file is read locally only
- No API key data is transmitted except to exchangeratesapi.io
- Keep your API key private and secure
- Regenerate key if compromised

## Support
- **CalcForge Issues**: Check application logs
- **API Issues**: Visit https://exchangeratesapi.io/documentation/
- **Account Issues**: Contact exchangeratesapi.io support
