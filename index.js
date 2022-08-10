const { Client, Intents, MessageEmbed } = require('discord.js');
const { token } = require('./config.json');
const { EmbedBuilder } = require('discord.js');

const client = new Client({
    partials: ["MESSAGE", "REACTION"],
    intents: [
        "GUILDS",
        "GUILD_MESSAGES",
        "GUILD_MESSAGE_REACTIONS",
    ],
});

client.once('ready', () => {
	console.log('Ready!');
});

client.on('messageCreate', async message => {

  const { exec } = require("child_process");

  async function jpegFilter(filter, path) {
    return new Promise((resolve, reject) => {
      exec(`./jpegFilter ${filter} ${path}`, (error, stdout, stderr) => {
        if (error) {
          reject(error);
        } else if (stderr) {
          console.log(stderr);
          reject(stderr);
        } else {
          resolve(stdout);
        }
      })
    })
  }

  if(message.content.startsWith("%")){
    command = message.content.slice(1, message.content.length);
    const fs = require('fs');
    const axios = require('axios').default;

    const embed = new MessageEmbed()
      .setColor(0xAAAA00)
      .setTitle("Welcome to Nels' Discord bot!")
      .setURL('https://github.com/Nels-Parenteau/discord-bot-image-manipulator')
      .setDescription("Please note that these commands only work with photos under the file format jpeg(.jpg). Get started by using a filter and an attatched image to the command! \n\n Commands are:\n\n - %help (This command)\n - %rot (This compresses)\n - %sharpen (This makes edges less blurry)\n - %border (Makes a bordered version of your image)\n - %text (Prints an ASCII version of your image)");
    
    if (command.startsWith("help")) {
      await message.channel.send({ embeds: [embed] });
    }

    if (command.startsWith("rot")) {
      await new Promise((resolve, reject) => {
        let writer = fs.createWriteStream('input.photo');
        let finish;
        try {finish = axios({
          method: 'get',
          url: message.attachments.first().url,
          responseType: 'stream'
        }).then((response) => {
          response.data.pipe(writer);
          writer.on('error', (err) => {
              reject(err);
          });
          writer.on('close', () => {
              resolve();
          });
        }) }
        catch (e) {
          console.log(e);
        }
        return finish
      }) 
      
      try {
        await jpegFilter("compress", "input.photo");
        await message.reply({files: ["./outputJpeg.jpg"]});
      }
      catch (e) {
        console.log(e);
      }
      
    }

    if (command.startsWith("border")) {
      await new Promise((resolve, reject) => {
        let writer = fs.createWriteStream('input.photo');
        let finish;
        try {finish = axios({
          method: 'get',
          url: message.attachments.first().url,
          responseType: 'stream'
        }).then((response) => {
          response.data.pipe(writer);
          writer.on('error', (err) => {
              reject(err);
          });
          writer.on('close', () => {
              resolve();
          });
        }) }
        catch (e) {
          console.log(e);
        }
        return finish
      }) 
      
      try {
        await jpegFilter("border", "input.photo");
        await message.reply({files: ["./outputJpeg.jpg"]});
      }
      catch (e) {
        console.log(e);
      }
    }

    if (command.startsWith("sharpen")) {
      await new Promise((resolve, reject) => {
        let writer = fs.createWriteStream('input.photo');
        let finish;
        try {finish = axios({
          method: 'get',
          url: message.attachments.first().url,
          responseType: 'stream'
        }).then((response) => {
          response.data.pipe(writer);
          writer.on('error', (err) => {
              reject(err);
          });
          writer.on('close', () => {
              resolve();
          });
        }) }
        catch (e) {
          console.log(e);
        }
        return finish
      }) 

      try {
        await jpegFilter("sharpen", "input.photo");
        await message.reply({files: ["./outputJpeg.jpg"]});
      }
      catch (e) {
        console.log(e);
      }
    }

    if (command.startsWith("text")) {
      await new Promise((resolve, reject) => {
        let writer = fs.createWriteStream('input.photo');
        let finish;
        try {finish = axios({
          method: 'get',
          url: message.attachments.first().url,
          responseType: 'stream'
        }).then((response) => {
          response.data.pipe(writer);
          writer.on('error', (err) => {
              reject(err);
          });
          writer.on('close', () => {
              resolve();
          });
        }) }
        catch (e) {
          console.log(e);
        }
        return finish
      }) 

      try {
        await jpegFilter("sharpen", "input.photo");
        await message.reply({files: ["./outputJpeg.jpg"]});
      }
      catch (e) {
        console.log(e);
      }
    }
  }
});

client.login(token);
