# weather
Weather station code using the Sentient Things IoT Node and Weather and Level Adapter

Requires the follows Integrations (webhooks) to be added to your Particle account:

{
    "event": "TSCreateChannel",
    "responseTopic": "{{PARTICLE_DEVICE_ID}}/hook-response/TSCreateChannelJson",
    "url": "https://api.thingspeak.com/channels.json",
    "requestType": "POST",
    "noDefaults": true,
    "rejectUnauthorized": true,
    "responseTemplate": "{\"i\":{{id}},\"w\":\"{{api_keys.0.api_key}}\", \"r\":\"{{api_keys.1.api_key}}\"}",
    "headers": {
        "Content-Type": "application/x-www-form-urlencoded"
    },
    "form": {
        "api_key": "YOUR_THINGSPEAK_USER_API_KEY",
        "description": "{{d}}",
        "elevation": "{{e}}",
        "field1": "{{1}}",
        "field2": "{{2}}",
        "field3": "{{3}}",
        "field4": "{{4}}",
        "field5": "{{5}}",
        "field6": "{{6}}",
        "field7": "{{7}}",
        "field8": "{{8}}",
        "latitude": "{{a}}",
        "longitude": "{{o}}",
        "name": "{{n}}",
        "public_flag": "{{f}}",
        "tags": "{{t}}",
        "url": "{{u}}",
        "metadata": "{{m}}"
    }
}

________________________________________
{
    "event": "TSWriteOneSetting",
    "responseTopic": "{{PARTICLE_DEVICE_ID}}/hook-response/TSWriteOneSetting",
    "url": "https://api.thingspeak.com/channels/{{c}}.json",
    "requestType": "PUT",
    "noDefaults": true,
    "rejectUnauthorized": true,
    "headers": {
        "Content-Type": "application/x-www-form-urlencoded"
    },
    "form": {
        "api_key": "YOUR_THINGSPEAK_USER_API_KEY",
        "{{n}}": "{{d}}"
    }
}
________________________________________
{
    "event": "TSBulkWriteCSV",
    "responseTopic": "{{PARTICLE_DEVICE_ID}}/hook-response/TSBulkWriteCSV",
    "url": "https://api.thingspeak.com/channels/{{c}}/bulk_update.csv",
    "requestType": "POST",
    "noDefaults": true,
    "rejectUnauthorized": true,
    "responseTemplate": "{{success}}",
    "headers": {
        "Content-Type": "application/x-www-form-urlencoded"
    },
    "form": {
        "write_api_key": "{{k}}",
        "time_format": "{{t}}",
        "updates": "{{d}}"
    }
}
