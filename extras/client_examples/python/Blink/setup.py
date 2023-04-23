# coding: utf-8
import requests

url = 'http://192.168.0.29/gpio15'


def main():
    headers = {"Content-Type": "text/plain; charset=utf-8"}
    response = requests.post(url, headers=headers, data="digitalWrite")
    print(response.content)


if __name__ == '__main__':
    main()
