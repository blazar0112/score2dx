#include <fstream>
#include <iostream>
#include <string>

#include "curl/curl.h"

#include "icl_s2/Common/IntegralRangeUsing.hpp"
#include "icl_s2/StdUtil/Find.hxx"
#include "icl_s2/Time/TimeUtilFormat.hxx"

#include "score2dx/Core/Core.hpp"
#include "score2dx/Core/JsonDefinition.hpp"
#include "score2dx/Csv/Csv.hpp"
#include "score2dx/Iidx/Version.hpp"
#include "score2dx/Score/PlayerScore.hpp"

namespace s2Time = icl_s2::Time;

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        auto begin = s2Time::Now();
        auto* curl = curl_easy_init();
        auto* slist = curl_slist_append(nullptr, "Iidxme-Api-Key: 295d293051a911ecbf630242ac130002");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        //std::string urlPrefix = "https://api.iidx.me/user/data/music?user=blazar&mid=";
        //std::string urlPrefix = "https://api.iidx.me/user/data/music?user=delmitz&mid=";

        //'' get list of versions with data.
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/versions?user=blazar");
        //'' get djdata, only need IIDX ID here.
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/data/djdata?user=blazara&ver=29");
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/data/music?user=blazar&mid=29000");
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/data/musiclist?user=blazar&ver=29&style=2");
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/data/score?user=blazar&ver=29&day=2021-11-20");
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api.iidx.me/user/data/music?user=blazar&mid=0001");

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](char* bufptr, std::size_t size, std::size_t nitems, void* userp)
            -> std::size_t
            {
                auto* buffer = reinterpret_cast<std::string*>(userp);
                buffer->append(bufptr, size*nitems);
                return size*nitems;
            }
        );


        std::string buffer;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        auto errorCode = curl_easy_perform(curl);
        if (errorCode!=CURLE_OK)
        {
            std::cout << "CURL error: " << errorCode << "\n";
            return 1;
        }

        std::cout << "response = " << buffer << std::endl;


        /*
        std::ofstream output{"iidxme_datatable_delmitz.json"};
        score2dx::Json dataTable;

        for (auto versionIndex: IndexRange{0, 30})
        {
            std::size_t noEntryCount = 0;
            for (auto musicIndex : IndexRange{1, 150})
            {
                auto musicId = score2dx::ToMusicId(versionIndex, musicIndex);
                auto musicIdString = score2dx::ToMusicIdString(musicId);
                auto iidxmeMid = musicIdString;
                if (versionIndex<10)
                {
                    iidxmeMid = musicIdString.substr(1);
                }
                auto url = urlPrefix+iidxmeMid;
                std::cout << "Check Music Data [" << iidxmeMid << "]" << std::endl;

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

                std::string buffer;

                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

                auto curlBegin = s2Time::Now();
                auto errorCode = curl_easy_perform(curl);

                if (errorCode!=CURLE_OK)
                {
                    std::cout << "CURL error: " << errorCode << "\n";
                    break;
                }

                auto json = score2dx::Json::parse(buffer);

                if (!icl_s2::Find(json, "code"))
                {
                    if (icl_s2::Find(json, "metadata"))
                    {
                        noEntryCount = 0;
                        dataTable[musicIdString] = json;
                    }
                    else
                    {
                        std::cout << "Get response but without metadata.\n";
                        break;
                    }
                }
                else
                {
                    noEntryCount++;
                }

                if (noEntryCount>10)
                {
                    break;
                }

                s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(curlBegin), musicIdString+" done.");
                while (s2Time::CountNs(curlBegin)<200'000'000)
                {
                    continue;
                }
            }
        }

        output << dataTable.dump();

        */

        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);

        std::cout << std::endl;
        s2Time::Print<std::chrono::milliseconds>(s2Time::CountNs(begin), "iidxme.exe");
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception:\n" << e.what() << std::endl;
        return 1;
    }
}
