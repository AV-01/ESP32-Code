import requests

url = "http://192.168.12.234/" + "write"

payload = {
    "filename" : "/temp_file_2.gcode",
    "foldername" : "",
    "content": "Testing, hello world\nmultiline test\nline 3"
}

response = requests.post(url, data=payload, timeout=10)
print(response.status_code)
print(response.text)