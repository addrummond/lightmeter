import pysimplesoap
import datetime
import os

PARTNER_ID = os.environ['MOUSER_PARTNER_ID']

client = pysimplesoap.client.SoapClient(wsdl="http://www.mouser.com/service/searchapi.asmx?WSDL", trace=True)
namespace = "http://api.mouser.com/service"
header = pysimplesoap.client.SimpleXMLElement("<Headers/>", namespace=namespace)
mh = header.add_child("MouserHeader")
mh['xmlns'] = namespace
ai = mh.add_child("AccountInfo")
ai.marshall("PartnerID", PARTNER_ID)
client['MouserHeader'] = mh
response = client.SearchByPartNumber(mouserPartNumber="foo")
print(response['SearchByPartNumberResult'])
