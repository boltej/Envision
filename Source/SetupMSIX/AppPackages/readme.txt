
1) Create a certificate 

PS C:\WINDOWS\system32> New-SelfSignedCertificate -Type Custom -Subject "CN=boltej" -KeyUsage DigitalSignature -FriendlyName "Envision cert" -CertStoreLocation "Cert:\CurrentUser\My" -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")


   PSParentPath: Microsoft.PowerShell.Security\Certificate::CurrentUser\My

Thumbprint                                Subject                                                                                              
----------                                -------                                                                                              
FD52C77B7785BB0E01649FF33CE4EA9B8B8CCA8C  CN=boltej                                                                                            

2) Export certificate file envcert.pfx

PS C:\WINDOWS\system32> $password = ConvertTo-SecureString -String lakers20lakers -Force -AsPlainText 
Export-PfxCertificate -cert "Cert:\CurrentUser\My\FD52C77B7785BB0E01649FF33CE4EA9B8B8CCA8C" -FilePath d:/Envision/envcert.pfx -Password $password


    Directory: D:\Envision


Mode                 LastWriteTime         Length Name                                                                                         
----                 -------------         ------ ----                                                                                         
-a----         12/6/2022   6:20 PM           2686 envcert.pfx                                                                                  

