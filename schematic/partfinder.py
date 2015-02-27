import pysimplesoap
import datetime
import os

PARTNER_ID = os.environ['MOUSER_PARTNER_ID']

def get_mouser_client():
    client = pysimplesoap.client.SoapClient(wsdl="http://www.mouser.com/service/searchapi.asmx?WSDL", trace=os.environ.get('DEBUG', False))
    namespace = "http://api.mouser.com/service"
    header = pysimplesoap.client.SimpleXMLElement("<Headers/>", namespace=namespace)
    mh = header.add_child("MouserHeader")
    mh['xmlns'] = namespace
    ai = mh.add_child("AccountInfo")
    ai.marshall("PartnerID", PARTNER_ID)
    client['MouserHeader'] = mh
    return client

def find_resistor(case, value):
    

if __name__ == '__main__':
    client = get_mouser_client()
    response = client.SearchByPartNumber(mouserPartNumber="667-ERJ-P6WF4701V")
    print(response['SearchByPartNumberResult'])
