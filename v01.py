from transformers import GPT2LMHeadModel, GPT2Tokenized, TextDataset, DataCollatorForLanguageModeling
from transformers import Trainer, TrainingArguments

#Load model and tokenizer
model_name = "distilgpt2"
tokenizer = GPT2Tokenizer.from_pretrained(model_name)
model = GPT2LMHeadModel.from_pretrained(model_name)
tokenizer.pad_token = tokenizer.eos_token

#Dataset
train_dataset = TextDaraset(
    tokenizer=tokenizer,
    file_path="data.txt",
    block_size=128
)

data_collator = DataCollatorForLanguageModeling(
    tokenizer=tokenizer,
    mlm=False
)

raining_args = TrainingArguments(
    output_dir="./output",
    overwrite_output_dir=True,
    num_train_epochs=3,
    per_device_train_batch_size=4,
    save_steps=500,
    save_total_limit=2,
)

trainer = Trainer(
    model=model,
    args=raining_args,
    data_collator=data_collator,
    train_dataset=train_dataset,
)

trainer.train()
trainer.save_model("./maboi")
#input_text = "..."
#inputs = tokenizer(input_text, return_tensors="pt")

#outputs = model.generate(**inputs, max_lenght=67, num_return_sequences=1)
#print(tokenizer.decode(outputs[0], skip_special_tokens=True))
