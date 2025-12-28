#!/bin/bash

# Get today's date in the format "Month Day, Year" (e.g., "December 27, 2025")
TODAYS_DATE=$(date +"%B %d, %Y")

# URL-encode the date string for use in the API query
ENCODED_DATE=$(echo "$TODAYS_DATE" | sed 's/ /%20/g' | sed 's/,/%2C/g')

# Construct the full API URL to fetch the wikitext content of the page
API_URL="en.wikiquote.org{ENCODED_DATE}&rvprop=content&format=json"

# Use curl to fetch the JSON data and then process it
curl -s "$API_URL" | \
  # Use a JSON parser (like 'jq' if available, otherwise basic text tools)
  # This example uses basic tools (grep, sed) to extract the relevant content
  # Note: A robust JSON parser is highly recommended for stability

  # Extract the wikitext content from the JSON response
  ggrep -oP '(?<=\"\*:).*?(?=\\")' | \
  
  # Clean up the wikitext to extract just the quote and author
  sed -e 's/<[^>]*>//g' -e 's/{{Qotd.*|quote=//' -e 's/|author=/\n— /' -e 's/}}/ /' -e 's/\[\[//g' -e 's/\]\]//g' -e 's/&quot;/"/g'

