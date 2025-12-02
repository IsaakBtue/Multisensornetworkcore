# Exact Replies to Send to the Other AI Assistant

Copy and paste these replies in order as you complete each step.

---

## Step 1: Initial Setup Confirmation

**Send this first:**

```
Authentication: Use option A – simple API key with X-API-Key header.

Payload format: Accept both single JSON object and array of objects. Normalize to array internally.

Table: Use table `telemetry` with columns:
- id bigserial primary key
- device_id text NOT NULL
- recorded_at timestamptz NULL
- received_at timestamptz NOT NULL DEFAULT now()
- payload jsonb NOT NULL
- temperature double precision NULL
- humidity double precision NULL
- co2 double precision NULL
- battery_voltage double precision NULL

Rate limiting: No special rate limiting or deduplication logic required.

Deployment: Show me the full TypeScript Edge Function code first with API key auth. After I review it, I will configure the API key as a secret and then deploy.
```

---

## Step 2: After Reviewing the Code

**After you've reviewed the Edge Function code and it looks good, send:**

```
The code looks good. I've set TELEMETRY_API_KEY as a secret in my Supabase project. Please deploy the Edge Function now.
```

**OR if you want to deploy manually, say:**

```
The code looks good. I'll deploy it manually using the Supabase CLI. Thank you!
```

---

## Step 3: If You Need Help Finding Your Edge Function URL

**After deployment, if you need the URL, ask:**

```
What is the URL format for my deployed Edge Function? I need to configure my ESP32 device to send data to it.
```

**Expected response:** They'll tell you it's:
```
https://<your-project-ref>.supabase.co/functions/v1/telemetry-ingest
```

---

## Step 4: If You Need to Test the Function

**To test if the function is working, ask:**

```
Can you help me test the Edge Function? I want to verify it's receiving data correctly. What's the best way to test it with a sample payload?
```

---

## Step 5: If You Encounter Errors

### If you get 401 Unauthorized:

**Send:**

```
I'm getting 401 Unauthorized errors. I've verified that:
- TELEMETRY_API_KEY secret is set in Supabase
- My ESP32 is sending the X-API-Key header with the correct value
- The header name matches exactly (case-sensitive)

Can you help me debug this?
```

### If you get database errors:

**Send:**

```
I'm getting database insert errors. The error message is: [paste error here]

My table schema is:
- id bigserial primary key
- device_id text NOT NULL
- received_at timestamptz NOT NULL DEFAULT now()
- payload jsonb NOT NULL
- temperature, humidity, co2, battery_voltage (all double precision, nullable)

Can you help me fix the insert query?
```

### If data isn't appearing in the table:

**Send:**

```
The Edge Function returns success (201 with inserted: 1), but I don't see any rows in my telemetry table. The function logs show no errors. What could be wrong?
```

---

## Step 6: If You Need to Update the Function

### To add more fields:

**Send:**

```
I need to add support for additional sensor fields in the payload. Can you update the Edge Function to accept and store:
- pressure (double precision)
- light_level (double precision)

These should be optional fields like the existing ones.
```

### To change authentication:

**Send:**

```
I want to switch from API key authentication to HMAC-SHA256 per-device signatures. Can you update the Edge Function to support HMAC authentication? I'll provide device secrets in a separate table.
```

---

## Step 7: Final Confirmation

**Once everything is working, you can say:**

```
Everything is working perfectly! The Edge Function is receiving data from my ESP32 devices and storing it in the telemetry table. Thank you for your help!
```

---

## Quick Reference: Common Issues

### Issue: Function not found (404)
**Reply:**
```
I'm getting 404 Not Found when calling the Edge Function. I'm using the URL: https://[my-project].supabase.co/functions/v1/telemetry-ingest

Is the function name correct? Should it be deployed differently?
```

### Issue: CORS errors
**Reply:**
```
I'm getting CORS errors when calling the Edge Function from my web browser. Can you add CORS headers to the function response?
```

### Issue: Timeout errors
**Reply:**
```
My ESP32 is getting timeout errors when sending data. The function seems to be taking too long. Can you optimize the insert operation or increase the timeout?
```

---

## Template for Custom Requests

If you need something specific, use this template:

```
I need to [describe what you need]. 

Current situation: [describe current state]
Expected behavior: [describe what should happen]
Error/issue: [paste any error messages]

Can you help me [specific request]?
```

---

## Notes

- Always be specific about what you need
- Include error messages when asking for help
- Mention what you've already tried
- Provide relevant details (table schema, function name, etc.)

