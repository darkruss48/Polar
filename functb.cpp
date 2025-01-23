#include "functb.h"
#include "gameplatform.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCoreApplication>
#include <QString>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <iostream>
#include <sstream>  // Nécessaire pour std::ostringstream
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>





#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QDebug>

QList<QJsonDocument> parseJsonToList(const QString &jsonString) {
    QList<QJsonDocument> jsonDocumentList;

    // Convert the input JSON string to a QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8());

    // Ensure the input JSON is an array
    if (!jsonDoc.isArray()) {
        // qWarning() << "Input JSON is not an array!";
        return jsonDocumentList;
    }

    // Extract the array from the document
    QJsonArray jsonArray = jsonDoc.array();

    // Iterate through the array and convert each object to a QJsonDocument
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            QJsonDocument doc(value.toObject());
            jsonDocumentList.append(doc);
        } else {
            // qWarning() << "Found a non-object in the JSON array!";
        }
    }

    // std::cout << "crampte" << jsonDocumentList[0].object()["users"].toObject()["id"].toString().toStdString() << std::endl;

    return jsonDocumentList;
}












#include <iostream>
#include <string>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

class GameAccount {
public:
    std::string unique_id;
    std::string identifier;
    std::string access_token;
    std::string secret;
};

#include <string>
#include <algorithm>
#include <iostream>
#include <vector>

std::string basic(const std::string& identifier) {
    std::string temp = identifier;
    if (identifier.find(':') == std::string::npos) {
        // remove next-lines
        temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
        // append blanks to make size of 160
        if (temp.size() < 159) {
            temp.append(159 - temp.size(), '\u0008');
        }
        // flip identifier
        QByteArray decoded = QByteArray::fromBase64(temp.c_str());
        QString decodedStr = QString::fromUtf8(decoded);
        QStringList parts = decodedStr.split(':');
        temp = parts[1].toStdString() + ":" + parts[0].toStdString();
    } else {
        // flip identifier
        QString tempStr = QString::fromStdString(temp);
        QStringList parts = tempStr.split(':');
        temp = parts[1].toStdString() + ":" + parts[0].toStdString();
    }
    // basic
    QByteArray encoded = QByteArray::fromStdString(temp).toBase64();
    return QString::fromUtf8(encoded).toStdString();
}

std::string basicAuth(const std::string &identifier) {
    // Implement the basic auth encoding here
    return "Basic " + basic(identifier); // Placeholder
}

QJsonObject postAuthSignIn(const std::string &authorization, const std::string &unique_id, const std::string &captcha_session_key = "") {
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://ishin-global.aktsk.com/auth/sign_in"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "Dalvik/2.1.0 (Linux; Android 7.0; SM-E7000)");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Authorization", QString::fromStdString(authorization).toUtf8());
    request.setRawHeader("Content-type", "application/json");
    request.setRawHeader("X-Platform", "android");
    request.setRawHeader("X-ClientVersion", "5.24.0-0640df5849d786f81804c8a902671531cba2e861820972d40d51589fbdebd67a");

    QJsonObject json;
    json["unique_id"] = QString::fromStdString(unique_id);
    if (!captcha_session_key.empty()) {
        json["captcha_session_key"] = QString::fromStdString(captcha_session_key);
    }
    json["bundle_id"] = "com.bandainamcogames.dbzdokkanww";
    json["device_token"] = "failed";
    json["reason"] = "NETWORK_ERROR: null";
    json["captcha_session_key"] = QJsonValue::Null;
    QJsonObject user_account;
    user_account["ad_id"] = "";
    user_account["unique_id"] = QString::fromStdString(unique_id);
    user_account["country"] = "FR";
    user_account["currency"] = "EUR";
    user_account["device"] = "SM";
    user_account["device_model"] = "SM-E7000";
    user_account["os_version"] = "7.0";
    user_account["platform"] = "android";
    json["user_account"] = user_account;
    /*
     {
            'ad_id': '',
            'unique_id': unique_id,
            'country': config.game_env.country,
            'currency': config.game_env.currency,
            'device': config.game_platform.device_name,
            'device_model': config.game_platform.device_model,
            'os_version': config.game_platform.os_version,
            'platform': config.game_platform.name
        }
    */

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = manager.post(request, data);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray response_data = reply->readAll();
    QJsonDocument response_doc = QJsonDocument::fromJson(response_data);
    return response_doc.object();
}

GameAccount login(GameAccount account) {
    std::string authorization = basicAuth(account.identifier);
    QJsonObject req = postAuthSignIn(authorization, account.unique_id);

    if (req.contains("captcha_url")) {
        std::string captcha_url = req["captcha_url"].toString().toStdString();
        std::string captcha_session_key = req["captcha_session_key"].toString().toStdString();
        std::cout << "Opening captcha in browser. Press ENTER once you have solved it..." << std::endl;
        // rajouter le fait que ca ouvre le navigateur avec le lien, etc...
        std::cin.get();
        req = postAuthSignIn(authorization, account.unique_id, captcha_session_key);
    }

    if (req.contains("access_token") && req.contains("secret")) {
        account.access_token = req["access_token"].toString().toStdString();
        account.secret = req["secret"].toString().toStdString();
        std::cout << "secret : " << account.secret << std::endl;
        std::cout << "access_token : " << account.access_token << std::endl;
        std::cout << "──[INFO] : Connexion réussie." << std::endl;
    } else {
        std::cout << "──[ERROR] : Login failed." << std::endl;
        std::cout << "Response : " << QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString() << std::endl;
    }

    return account;
}














QString md5(const QString& input) {
    QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex();
}

QString base64_encode(const QByteArray& input) {
    return input.toBase64();
}

QString functb::mac(const QString& url, int port, const QString& method, const QString& action, QString secret, QString access_token) {
    // Obtenir le timestamp actuel
    QString ts = QString::number(QDateTime::currentSecsSinceEpoch());

    // Générer le nonce
    QString nonce = ts + ":" + md5(ts);

    // Construire la valeur
    QString value = ts + "\n" + nonce + "\n" + method + "\n" + action + "\n" +
                    url.mid(8) + "\n" + QString::number(port) + "\n\n";

    // Calculer le HMAC-SHA256
    QByteArray secretBytes = secret.toUtf8();
    QByteArray valueBytes = value.toUtf8();
    QByteArray hmacResult = QMessageAuthenticationCode::hash(valueBytes, secretBytes, QCryptographicHash::Sha256);

    // Encoder en Base64
    QString mac_encoded = base64_encode(hmacResult);

    // Construire la chaîne finale
    QString final = "MAC id=\"" + access_token + "\" nonce=\"" + nonce + "\" ts=\"" + ts + "\" mac=\"" + mac_encoded + "\"";

    return final;
}

std::string functb::ver_code = "5.25.0-daa3501ede0d27bd73ecea640bf67a1f11816e94549d40dc29ddf6efe07476ff";
std::string functb::secret = "";
std::string functb::access_token = "";
std::string functb::identifier = "poyo :3";


functb::functb() {}

void functb::getHeader(QNetworkRequest &request)
{
    GamePlatform android_platform(
        "android",
        "Dalvik/2.1.0 (Linux; Android 7.0; SM-E7000)",
        "SM",
        "SM-E7000",
        "7.0"
        );
    /*request.setRawHeader("X-Platform", "android");
    request.setRawHeader("X-ClientVersion", android_platform.user_agent.c_str());
    request.setRawHeader("X-Language", "en");
    request.setRawHeader("X-UserID", "////");
    */

    request.setRawHeader("User-Agent", android_platform.user_agent.c_str());
    request.setRawHeader("Accept", "*/*");
    // request.setRawHeader("Authorization", "###########"); // mis dans le fichier principal, car besoin fonction mac
    request.setRawHeader("Content-type", "application/json");
    request.setRawHeader("X-Platform", "android");
    request.setRawHeader("X-AssetVersion", "////");
    request.setRawHeader("X-DatabaseVersion", "////");
    request.setRawHeader("X-ClientVersion", ver_code.c_str());
    request.setRawHeader("X-Language", "en");

}

void functb::connect(Ui::MainWindow *ui)
{
    ui->boitetext->clear();
    // Requete

    GameAccount account;
    account.unique_id = "1ea06bda-1fcd-47c0-a3d6-2aaffcfe692e:cb312a3d";
    account.identifier = "vr6a+hI9gUXnWUPaRBc/QHqng4qoxGqT1i8xvth1V94ykKWIy9nU13I1Ywps8qBOH9tmoHmDsT5AQvoAoW0akg==:RcznD3hEGk+hSHUyTwuO1g==";

    account = login(account);

    std::cout << "Access Token: " << account.access_token << std::endl;
    std::cout << "Secret: " << account.secret << std::endl;

    // Les mettre dans les variables statiques
    functb::access_token = QString::fromStdString(account.access_token).toUtf8();
    functb::secret = QString::fromStdString(account.secret).toUtf8();

    // Mettre secret et access_token dans le static de functb
    /*

    // Création d'un gestionnaire réseau
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    // On fait une boucle for afin d'avoir toutes les datas
    // URL cible
    std::ostringstream url_;
    url_ << "https://ishin-global.aktsk.com/budokais/" << ui->id_tb->currentText().toInt() <<"/rankings?per=100&page=1";
    QString url__ = QString::fromStdString(url_.str());
    QUrl url(url__);

    // Préparation de la requête
    QNetworkRequest request(url);
    // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // request.setRawHeader("X-Platform", "android");
    // request.setRawHeader("X-ClientVersion", functb::ver_code.c_str());
    // request.setRawHeader("X-Language", "en");
    // request.setRawHeader("X-UserID", "////");

    // In-game data
    // std::string secret = "KOD4guawptviWPJ0cIR/RFHPSZ6Zgf89tHzEQn6N3QCGraXt3Sex+HTRVz/apxH5NBPfdC+NwDyASYo+Z3TSGw==";
    // std::string access_token = "2dpRGLBbJsTnPwpn1S9BuK6gIaHAko6UbNvgF59Bi64Cs3DLO5gf4xXlPFnEZcsm5YoQ2XdE30TGoTdWUqavXA==";

    // Optionnel : Ajouter des paramètres JSON pour une requête POST
    QJsonObject jsonBody;
    // jsonBody["param1"] = "valeur1";
    // jsonBody["param2"] = "valeur2";

    QString lien = "https://ishin-global.aktsk.com";
    int port = 443;
    QString method = "GET";
    std::ostringstream action_;
    std::cout << ui->id_tb->currentText().toInt() << std::endl;
    action_ << "https://ishin-global.aktsk.com/budokais/" << ui->id_tb->currentText().toInt() <<"/rankings?per=100&page=1";
    QString action = QString::fromStdString(action_.str());
    QString secret = "/r3DW6y+s+zi20Zq0axMDyPRchs4uFju6KcHV6Pf1AnmzxIkSXJq6G5G4WLNKkHVBmFNCE4WhxvtUUnqGER0Ng==";
    QString access_token = "y8LZISDyk0JTe85EWDPeRj54GWya/ot+10rysRmjgcdibPSWRSZnFPaaR5FGGpmfbV0aE8xxNOkhBzvyLOR3jw==";

    QString mac_str = functb::mac(lien, port, method, action, secret, access_token);

    std::cout << "mac_str : " << mac_str.toStdString() << std::endl;
    // functb::getHeader(request);
    request.setRawHeader("User-Agent", "Dalvik/2.1.0 (Linux; Android 7.0; SM-E7000)");
    */
    // request.setRawHeader("Accept", "*/*"); AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    /*
    request.setRawHeader("Authorization", mac_str.toUtf8());
    request.setRawHeader("Content-type", "application/json");
    request.setRawHeader("X-Platform", "android");
    request.setRawHeader("X-AssetVersion", "////");
    request.setRawHeader("X-DatabaseVersion", "////");
    request.setRawHeader("X-ClientVersion", functb::ver_code.c_str());
    request.setRawHeader("X-Language", "en");
    // Conversion en QByteArray
    QByteArray bodyData = QJsonDocument(jsonBody).toJson();
    // ui->boitetext->append();
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    // afficher toutes les infos de la requete : headers, url, etc...
    // url
    ui->boitetext->append("URL : " + url__);
    // headers
    QList<QByteArray> headers = request.rawHeaderList();
    ui->boitetext->append("Headers : ");
    for (int i = 0; i < headers.size(); i++) {
        ui->boitetext->append(headers.at(i) + " : " + request.rawHeader(headers.at(i)));
        std::cout << "Headers : \"" << headers.at(i).toStdString() << "\" : \"" << request.rawHeader(headers.at(i)).toStdString() << "\"" << std::endl;
    }

    // Envoyer une requête GET
    QNetworkReply *reply = manager->get(request, bodyData);
    // ui->boitetext->append("b");

    // Gérer la réponse (asynchrone)
    QObject::connect(reply, &QNetworkReply::finished, [reply, ui, request]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Récupérer la réponse en texte brut
            QByteArray responseBytes = reply->readAll();
            //qDebug() << "Réponse brute:" << responseBytes;
            ui->boitetext->append("Réponse brute reçue : \"" + QString(responseBytes) + "\"");

            // Convertir la réponse en JSON
            QJsonDocument jsonResponse = QJsonDocument::fromJson(responseBytes);
            if (!jsonResponse.isNull()) {
                // qDebug() << "Réponse JSON:" << jsonResponse;
                // ui->boitetext->append("Réponse JSON formatée : " + QString(jsonResponse.toJson(QJsonDocument::Indented)));
            } else {
                // qWarning() << "Erreur lors de la conversion en JSON";
                ui->boitetext->append("Erreur lors de la conversion en JSON");
            }
        } else {
            // Gestion des erreurs
            // qWarning() << "Erreur réseau:" << reply->errorString();
            ui->boitetext->append("Erreur réseau : " + reply->errorString());
            QByteArray responseBytes = reply->readAll();
            ui->boitetext->append("Réponse brute reçue : \"" + QString(responseBytes) + "\"");
            // récupérer les headers
            QList<QByteArray> headers = request.rawHeaderList();
            // ui->boitetext->append("Headers : ");
            for (int i = 0; i < headers.size(); i++) {
                // ui->boitetext->append(headers.at(i) + " : " + reply->rawHeader(headers.at(i)));
                std::cout << "Headers : \"" << headers.at(i).toStdString() << "\" : \"" << reply->rawHeader(headers.at(i)).toStdString() << "\"" << std::endl;
            }
        }

        // test
        QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        if (statusCode.isValid()) {
            int httpStatus = statusCode.toInt();
            ui->boitetext->append("Code de statut HTTP: " + QString::number(httpStatus));

            if (httpStatus != 200) {
                ui->boitetext->append("Le serveur a répondu avec un code d'erreur HTTP.");
            }
        } else {
            ui->boitetext->append("Impossible de récupérer le code de statut HTTP.");
        }

        reply->deleteLater();
    });
    */
}

QJsonObject functb::pologet()
{

    QNetworkAccessManager manager;
    QUrl a = "http://86.247.232.153:41766/get-user?identifier=" + QString::fromStdString(functb::identifier);
    /*
    Hey
    If you wondering why we are sending a GET request to a weird IP, you are right to wonder.
    This is a server that polo (goat) setup to get information about the users.
    The server is auto-updated directly from akatsuki's servers.
    You can execute this command in your terminal (cmd on Windows) :
    curl -X GET "http://86.247.232.153:41766/get-user?identifier=PUT_YOUR_DOKKAN_ID_OR_NAME"
    and you will get your user information.
    Hope this helps.
    */
    QNetworkRequest request(a);
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray response_data = reply->readAll();
    QJsonDocument response_doc = QJsonDocument::fromJson(response_data);
    if (response_doc.isNull()) {
        //
        return QJsonObject();
    }
    if (response_doc.object().isEmpty())
    {
        // Convertir
        QList<QJsonDocument> jsonDocumentList = parseJsonToList(QString::fromUtf8(response_data));
        // Convert QList<QJsonDocument> to QList<QVariant>
        QList<QVariant> variantList;
        for (const QJsonDocument &doc : jsonDocumentList) {
            variantList.append(doc.toVariant());
        }
        // Créer un nouveau json
        QJsonObject new_json;
        // Mettre la liste dans un element "users"
        new_json["users"] = QJsonArray::fromVariantList(variantList);
        // afficher le nombre d'users
        return new_json;
    }
    return response_doc.object();
}
QJsonObject functb::pologettop()
{

    QNetworkAccessManager manager;
    QUrl a = QString("http://86.247.232.153:41766/get-top100");
    /*
    Hey
    If you wondering why we are sending a GET request to a weird IP, you are right to wonder.
    This is a server that polo (goat) setup to get information about the users.
    The server is auto-updated directly from akatsuki's servers.
    You can execute this command in your terminal (cmd on Windows) :
    curl -X GET "http://86.247.232.153:41766/get-user?identifier=PUT_YOUR_DOKKAN_ID_OR_NAME"
    and you will get your user information.
    Hope this helps.
    */
    QNetworkRequest request(a);
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray response_data = reply->readAll();
    QJsonDocument response_doc = QJsonDocument::fromJson(response_data);
    if (response_doc.isNull()) {
        //
        return QJsonObject();
    }
    if (response_doc.object().isEmpty())
    {
        // Convertir
        QList<QJsonDocument> jsonDocumentList = parseJsonToList(QString::fromUtf8(response_data));
        // Convert QList<QJsonDocument> to QList<QVariant>
        QList<QVariant> variantList;
        for (const QJsonDocument &doc : jsonDocumentList) {
            variantList.append(doc.toVariant());
        }
        // Créer un nouveau json
        QJsonObject new_json;
        // Mettre la liste dans un element "users"
        new_json["top"] = QJsonArray::fromVariantList(variantList);
        // afficher le nombre d'users
        return new_json;
    }
    return response_doc.object();
}
