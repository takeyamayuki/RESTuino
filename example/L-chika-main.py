# coding: utf-8
import requests
import time

url = 'http://192.168.3.20/gpio15'
headers = {"Content-Type": "text/plain; charset=utf-8"}


def main():
    while(1):
        response = requests.put(url, headers=headers, data="switch")
        print(response.content)
        time.sleep(1)


if __name__ == '__main__':
    main()
